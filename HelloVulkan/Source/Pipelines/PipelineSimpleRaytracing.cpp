#include "PipelineSimpleRaytracing.h"
#include "VulkanShader.h"
#include "VulkanUtility.h"
#include "Configs.h"

uint32_t AlignedSize(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

size_t AlignedSize(size_t value, size_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

PipelineSimpleRaytracing::PipelineSimpleRaytracing(VulkanDevice& vkDev) :
	PipelineBase(
		vkDev,
		{
			// TODO Is this correct?
			.type_ = PipelineType::GraphicsOnScreen
		})
{
	cameraUboBuffer_.CreateBuffer(
		vkDev,
		sizeof(CameraUBO),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU);

	CreateBLAS(vkDev);
	CreateTLAS(vkDev);

	CreateStorageImage(vkDev);
	CreateDescriptor(vkDev);
	CreateRayTracingPipeline(vkDev);
	CreateShaderBindingTable(vkDev);
}

PipelineSimpleRaytracing::~PipelineSimpleRaytracing()
{
	cameraUboBuffer_.Destroy();

	vkDestroyDescriptorSetLayout(device_, descriptorLayout_, nullptr);
	vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
	storageImage_.Destroy(device_);
	blas_.Destroy(device_);
	vkDestroyAccelerationStructureKHR(device_, blas_.handle_, nullptr);
	tlas_.Destroy(device_);
	vkDestroyAccelerationStructureKHR(device_, tlas_.handle_, nullptr);
	vertexBuffer_.Destroy();
	indexBuffer_.Destroy();
	transformBuffer_.Destroy();
	raygenShaderBindingTable_.Destroy();
	missShaderBindingTable_.Destroy();
	hitShaderBindingTable_.Destroy();
}

void PipelineSimpleRaytracing::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer)
{
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR properties = vkDev.GetRayTracingPipelineProperties();

	const uint32_t handleSizeAligned = AlignedSize(properties.shaderGroupHandleSize, properties.shaderGroupHandleAlignment);

	VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry =
	{
		.deviceAddress = GetBufferDeviceAddress(vkDev, raygenShaderBindingTable_.buffer_),
		.stride = handleSizeAligned,
		.size = handleSizeAligned
	};

	VkStridedDeviceAddressRegionKHR missShaderSbtEntry =
	{
		.deviceAddress = GetBufferDeviceAddress(vkDev, missShaderBindingTable_.buffer_),
		.stride = handleSizeAligned,
		.size = handleSizeAligned,
	};

	VkStridedDeviceAddressRegionKHR hitShaderSbtEntry =
	{
		.deviceAddress = GetBufferDeviceAddress(vkDev, hitShaderBindingTable_.buffer_),
		.stride = handleSizeAligned,
		.size = handleSizeAligned
	};
	VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

	/*
		Dispatch the ray tracing commands
	*/
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline_);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout_, 0, 1, &descriptorSet_, 0, 0);

	vkCmdTraceRaysKHR(
		commandBuffer,
		&raygenShaderSbtEntry,
		&missShaderSbtEntry,
		&hitShaderSbtEntry,
		&callableShaderSbtEntry,
		vkDev.GetFrameBufferWidth(),
		vkDev.GetFrameBufferHeight(),
		1);

	/*
		Copy ray tracing output to swap chain image
	*/
	uint32_t swapchainIndex = vkDev.GetCurrentSwapchainImageIndex();

	storageImage_.TransitionImageLayoutCommand(
		commandBuffer,
		storageImage_.imageFormat_, 
		VK_IMAGE_LAYOUT_GENERAL, 
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		1u,
		1u);

	TransitionImageLayoutCommand(
		commandBuffer, 
		vkDev.GetSwapchainImage(swapchainIndex),
		vkDev.GetSwapchainImageFormat(),
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1u,
		1u);
	

	VkImageCopy copyRegion{};
	copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.srcOffset = { 0, 0, 0 };
	copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.dstOffset = { 0, 0, 0 };
	copyRegion.extent = { vkDev.GetFrameBufferWidth(), vkDev.GetFrameBufferHeight(), 1 };
	vkCmdCopyImage(commandBuffer, 
		storageImage_.image_, 
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
		vkDev.GetSwapchainImage(swapchainIndex), 
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
		1, 
		&copyRegion);
	
	TransitionImageLayoutCommand(
		commandBuffer,
		vkDev.GetSwapchainImage(swapchainIndex),
		vkDev.GetSwapchainImageFormat(),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		1u,
		1u);
	
	// Transition ray tracing output image back to general layout
	storageImage_.TransitionImageLayoutCommand(commandBuffer, 
		storageImage_.imageFormat_, 
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
		VK_IMAGE_LAYOUT_GENERAL);
}

void PipelineSimpleRaytracing::CreateDescriptor(VulkanDevice& vkDev)
{
	// Pool
	std::vector<VkDescriptorPoolSize> poolSizes = {
			{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
	};

	VkDescriptorPoolCreateInfo poolInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = 1u,
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.data(),
	};

	VK_CHECK(vkCreateDescriptorPool(vkDev.GetDevice(), &poolInfo, nullptr, &descriptorPool_));

	// Layout 
	VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding =
	{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR
	};

	VkDescriptorSetLayoutBinding resultImageLayoutBinding =
	{
		.binding = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR
	};

	VkDescriptorSetLayoutBinding uniformBufferBinding =
	{
		.binding = 2,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR
	};

	std::vector<VkDescriptorSetLayoutBinding> bindings({
		accelerationStructureLayoutBinding,
		resultImageLayoutBinding,
		uniformBufferBinding
		});

	VkDescriptorSetLayoutCreateInfo descriptorSetlayoutCI =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data(),
	};
	VK_CHECK(vkCreateDescriptorSetLayout(vkDev.GetDevice(), &descriptorSetlayoutCI, nullptr, &descriptorLayout_));

	// Set
	VkDescriptorSetAllocateInfo allocateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool_,
		.descriptorSetCount = 1u,
		.pSetLayouts = &descriptorLayout_
	};

	VK_CHECK(vkAllocateDescriptorSets(vkDev.GetDevice(), &allocateInfo, &descriptorSet_));

	// Binding 0
	VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo =
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
		.accelerationStructureCount = 1u,
		.pAccelerationStructures = &tlas_.handle_,
	};
	VkWriteDescriptorSet accelerationStructureWrite =
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		// The specialized acceleration structure descriptor has to be chained
		.pNext = &descriptorAccelerationStructureInfo,
		.dstSet = descriptorSet_,
		.dstBinding = 0u,
		.descriptorCount = 1u,
		.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
	};

	VkDescriptorImageInfo storageImageDescriptor =
	{
		.imageView = storageImage_.imageView_,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};

	VkWriteDescriptorSet resultImageWrite =
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptorSet_,
		.dstBinding = 1u,
		.descriptorCount = 1u,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.pImageInfo = &storageImageDescriptor,
	};

	VkDescriptorBufferInfo uboBufferInfo = { cameraUboBuffer_.buffer_, 0, sizeof(CameraUBO)};
	
	VkWriteDescriptorSet uniformBufferWrite =
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = descriptorSet_,
		.dstBinding = 2u,
		.descriptorCount = 1u,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.pBufferInfo = &uboBufferInfo
	};
	
	std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
		accelerationStructureWrite,
		resultImageWrite,
		uniformBufferWrite
	};
	vkUpdateDescriptorSets(vkDev.GetDevice(), 
		static_cast<uint32_t>(writeDescriptorSets.size()),
		writeDescriptorSets.data(), 
		0, 
		VK_NULL_HANDLE);
}

void PipelineSimpleRaytracing::CreateStorageImage(VulkanDevice& vkDev)
{
	storageImage_.CreateImage(
		vkDev,
		vkDev.GetFrameBufferWidth(),
		vkDev.GetFrameBufferHeight(),
		1u,
		1u,
		vkDev.GetSwapchainImageFormat(),
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY
	);

	storageImage_.CreateImageView(
		vkDev,
		vkDev.GetSwapchainImageFormat(),
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_2D,
		1u,
		1u);

	storageImage_.TransitionImageLayout(vkDev, 
		storageImage_.imageFormat_, 
		VK_IMAGE_LAYOUT_UNDEFINED, 
		VK_IMAGE_LAYOUT_GENERAL);
}

void PipelineSimpleRaytracing::CreateRayTracingPipeline(VulkanDevice& vkDev)
{
	// Pipeline layout
	const VkPipelineLayoutCreateInfo pipelineLayoutCI =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &descriptorLayout_
	};
	VK_CHECK(vkCreatePipelineLayout(vkDev.GetDevice(), &pipelineLayoutCI, nullptr, &pipelineLayout_));

	// Shaders
	const std::vector<std::string> shaderFiles =
	{
		AppConfig::ShaderFolder + "SimpleRaytracing//RayGen.rgen",
		AppConfig::ShaderFolder + "SimpleRaytracing//Miss.rmiss",
		AppConfig::ShaderFolder + "SimpleRaytracing//ClosestHit.rchit"
	};

	std::vector<VulkanShader> shaderModules(shaderFiles.size());
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages(shaderFiles.size());
	for (size_t i = 0; i < shaderFiles.size(); i++)
	{
		const char* file = shaderFiles[i].c_str();
		VK_CHECK(shaderModules[i].Create(vkDev.GetDevice(), file));
		const VkShaderStageFlagBits stage = GetShaderStageFlagBits(file);
		shaderStages[i] = shaderModules[i].GetShaderStageInfo(stage, "main");
	}

	// TODO Shader groups
	shaderGroups_ =
	{
		{
			.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
			.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
			.generalShader = 0u,
			.closestHitShader = VK_SHADER_UNUSED_KHR,
			.anyHitShader = VK_SHADER_UNUSED_KHR,
			.intersectionShader = VK_SHADER_UNUSED_KHR
		},
		{
			.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
			.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
			.generalShader = 1u,
			.closestHitShader = VK_SHADER_UNUSED_KHR,
			.anyHitShader = VK_SHADER_UNUSED_KHR,
			.intersectionShader = VK_SHADER_UNUSED_KHR,
		},
		{
			.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
			.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
			.generalShader = VK_SHADER_UNUSED_KHR,
			.closestHitShader = 2u,
			.anyHitShader = VK_SHADER_UNUSED_KHR,
			.intersectionShader = VK_SHADER_UNUSED_KHR
		}
	};

	// Pipeline
	VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCI =
	{
		.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
		.stageCount = static_cast<uint32_t>(shaderStages.size()),
		.pStages = shaderStages.data(),
		.groupCount = static_cast<uint32_t>(shaderGroups_.size()),
		.pGroups = shaderGroups_.data(),
		.maxPipelineRayRecursionDepth = 1,
		.layout = pipelineLayout_
	};
	VK_CHECK(vkCreateRayTracingPipelinesKHR(
		vkDev.GetDevice(), 
		VK_NULL_HANDLE, 
		VK_NULL_HANDLE, 
		1, 
		&rayTracingPipelineCI, 
		nullptr, 
		&pipeline_));

	for (VulkanShader& s : shaderModules)
	{
		s.Destroy(vkDev.GetDevice());
	}
}

void PipelineSimpleRaytracing::CreateBLAS(VulkanDevice& vkDev)
{
	// Setup vertices for a single triangle
	struct Vertex
	{
		float pos[3];
	};
	std::vector<Vertex> vertices = {
		{ {  1.0f,  1.0f, 0.0f } },
		{ { -1.0f,  1.0f, 0.0f } },
		{ {  0.0f, -1.0f, 0.0f } }
	};

	// Setup indices
	std::vector<uint32_t> indices = { 0, 1, 2 };
	indexCount_ = static_cast<uint32_t>(indices.size());

	// Setup identity transform matrix
	VkTransformMatrixKHR transformMatrix = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f
	};

	vertexBuffer_.CreateBuffer(
		vkDev,
		vertices.size() * sizeof(Vertex),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	vertexBuffer_.UploadBufferData(vkDev, 0, vertices.data(), vertices.size() * sizeof(Vertex));

	indexBuffer_.CreateBuffer(
		vkDev,
		indices.size() * sizeof(uint32_t),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	indexBuffer_.UploadBufferData(vkDev, 0, indices.data(), indices.size() * sizeof(uint32_t));

	transformBuffer_.CreateBuffer(
		vkDev,
		sizeof(VkTransformMatrixKHR),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	transformBuffer_.UploadBufferData(vkDev, 0, &transformMatrix, sizeof(VkTransformMatrixKHR));

	VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
	VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
	VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};

	vertexBufferDeviceAddress.deviceAddress = GetBufferDeviceAddress(vkDev, vertexBuffer_.buffer_);
	indexBufferDeviceAddress.deviceAddress = GetBufferDeviceAddress(vkDev, indexBuffer_.buffer_);
	transformBufferDeviceAddress.deviceAddress = GetBufferDeviceAddress(vkDev, transformBuffer_.buffer_);

	// Build
	VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
	accelerationStructureGeometry.geometry.triangles.maxVertex = 2;
	accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(Vertex);
	accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;
	accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
	accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;
	accelerationStructureGeometry.geometry.triangles.transformData = transformBufferDeviceAddress;

	// Get size info
	VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
	accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationStructureBuildGeometryInfo.geometryCount = 1;
	accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

	const uint32_t numTriangles = 1;
	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
	accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(
		vkDev.GetDevice(),
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&accelerationStructureBuildGeometryInfo,
		&numTriangles,
		&accelerationStructureBuildSizesInfo);

	CreateAccelerationStructureBuffer(vkDev, blas_, accelerationStructureBuildSizesInfo);

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = blas_.buffer_,
		.size = accelerationStructureBuildSizesInfo.accelerationStructureSize,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
	};
	vkCreateAccelerationStructureKHR(vkDev.GetDevice(), &accelerationStructureCreateInfo, nullptr, &blas_.handle_);

	// Create a small scratch buffer used during build of the bottom level acceleration structure
	ScratchBuffer scratchBuffer = CreateScratchBuffer(vkDev, accelerationStructureBuildSizesInfo.buildScratchSize);

	VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
		.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
		.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		.dstAccelerationStructure = blas_.handle_,
		.geometryCount = 1,
		.pGeometries = &accelerationStructureGeometry
	};
	accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress_;

	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo =
	{
		.primitiveCount = numTriangles,
		.primitiveOffset = 0,
		.firstVertex = 0,
		.transformOffset = 0,
	};
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = 
	{ &accelerationStructureBuildRangeInfo };

	// Build the acceleration structure on the device via a one-time command buffer submission
	// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), 
	// but we prefer device builds
	VkCommandBuffer commandBuffer = vkDev.BeginOneTimeGraphicsCommand();
	vkCmdBuildAccelerationStructuresKHR(
		commandBuffer,
		1,
		&accelerationBuildGeometryInfo,
		accelerationBuildStructureRangeInfos.data());
	vkDev.EndOneTimeGraphicsCommand(commandBuffer);

	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
		.accelerationStructure = blas_.handle_,
	};
	blas_.deviceAddress_ = vkGetAccelerationStructureDeviceAddressKHR(vkDev.GetDevice(), &accelerationDeviceAddressInfo);

	DeleteScratchBuffer(vkDev, scratchBuffer);
}

void PipelineSimpleRaytracing::CreateTLAS(VulkanDevice& vkDev)
{
	VkTransformMatrixKHR transformMatrix = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f };

	VkAccelerationStructureInstanceKHR instance =
	{
		.transform = transformMatrix,
		.instanceCustomIndex = 0,
		.mask = 0xFF,
		.instanceShaderBindingTableRecordOffset = 0,
		.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
		.accelerationStructureReference = blas_.deviceAddress_
	};

	VulkanBuffer instancesBuffer;
	instancesBuffer.CreateBuffer(vkDev,
		sizeof(VkAccelerationStructureInstanceKHR),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_CPU_TO_GPU);

	VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress =
	{
		.deviceAddress = GetBufferDeviceAddress(vkDev, instancesBuffer.buffer_)
	};

	VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
	accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

	// Get size info
	/*
	The pSrcAccelerationStructure, dstAccelerationStructure, and mode members of pBuildInfo are ignored. 
	Any VkDeviceOrHostAddressKHR members of pBuildInfo are ignored by this command, 
	except that the hostAddress member of VkAccelerationStructureGeometryTrianglesDataKHR::transformData will 
	be examined to check if it is NULL.
	*/
	VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
		.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
		.geometryCount = 1,
		.pGeometries = &accelerationStructureGeometry
	};
	uint32_t primitive_count = 1;

	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
	};
	vkGetAccelerationStructureBuildSizesKHR(
		vkDev.GetDevice(),
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&accelerationStructureBuildGeometryInfo,
		&primitive_count,
		&accelerationStructureBuildSizesInfo);

	CreateAccelerationStructureBuffer(vkDev, tlas_, accelerationStructureBuildSizesInfo);

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = tlas_.buffer_,
		.size = accelerationStructureBuildSizesInfo.accelerationStructureSize,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR
	};
	vkCreateAccelerationStructureKHR(vkDev.GetDevice(), &accelerationStructureCreateInfo, nullptr, &tlas_.handle_);

	// Create a small scratch buffer used during build of the top level acceleration structure
	ScratchBuffer scratchBuffer = CreateScratchBuffer(vkDev, accelerationStructureBuildSizesInfo.buildScratchSize);

	VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
		.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
		.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		.dstAccelerationStructure = tlas_.handle_,
		.geometryCount = 1,
		.pGeometries = &accelerationStructureGeometry,
	};
	accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress_;

	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo =
	{
		.primitiveCount = 1,
		.primitiveOffset = 0,
		.firstVertex = 0,
		.transformOffset = 0,
	};
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

	// Build the acceleration structure on the device via a one-time command buffer submission
	// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), 
	// but we prefer device builds
	VkCommandBuffer commandBuffer = vkDev.BeginOneTimeGraphicsCommand();
	vkCmdBuildAccelerationStructuresKHR(
		commandBuffer,
		1,
		&accelerationBuildGeometryInfo,
		accelerationBuildStructureRangeInfos.data());
	vkDev.EndOneTimeGraphicsCommand(commandBuffer);

	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
		.accelerationStructure = tlas_.handle_
	};
	tlas_.deviceAddress_ = vkGetAccelerationStructureDeviceAddressKHR(vkDev.GetDevice(), &accelerationDeviceAddressInfo);

	DeleteScratchBuffer(vkDev, scratchBuffer);
	instancesBuffer.Destroy();
}

void PipelineSimpleRaytracing::CreateAccelerationStructureBuffer(
	VulkanDevice& vkDev, 
	AccelerationStructure& accelerationStructure, 
	VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo)
{
	VkBufferCreateInfo bufferCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = buildSizeInfo.accelerationStructureSize,
		.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
	};
	VK_CHECK(vkCreateBuffer(vkDev.GetDevice(), &bufferCreateInfo, nullptr, &accelerationStructure.buffer_));
	
	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(vkDev.GetDevice(), accelerationStructure.buffer_, &memoryRequirements);

	VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
		.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR
	};

	VkMemoryAllocateInfo memoryAllocateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = &memoryAllocateFlagsInfo,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = vkDev.GetMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	};

	VK_CHECK(vkAllocateMemory(vkDev.GetDevice(), &memoryAllocateInfo, nullptr, &accelerationStructure.memory_));
	VK_CHECK(vkBindBufferMemory(vkDev.GetDevice(), accelerationStructure.buffer_, accelerationStructure.memory_, 0));
}

ScratchBuffer PipelineSimpleRaytracing::CreateScratchBuffer(VulkanDevice& vkDev, VkDeviceSize size)
{
	ScratchBuffer scratchBuffer{};

	VkBufferCreateInfo bufferCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
	};
	VK_CHECK(vkCreateBuffer(vkDev.GetDevice(), &bufferCreateInfo, nullptr, &scratchBuffer.handle_));

	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(vkDev.GetDevice(), scratchBuffer.handle_, &memoryRequirements);

	VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
		.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR
	};

	// TODO VMA
	VkMemoryAllocateInfo memoryAllocateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = &memoryAllocateFlagsInfo,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = vkDev.GetMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
	};
	VK_CHECK(vkAllocateMemory(vkDev.GetDevice(), &memoryAllocateInfo, nullptr, &scratchBuffer.memory_));
	VK_CHECK(vkBindBufferMemory(vkDev.GetDevice(), scratchBuffer.handle_, scratchBuffer.memory_, 0));

	VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = scratchBuffer.handle_
	};
	scratchBuffer.deviceAddress_ = vkGetBufferDeviceAddressKHR(vkDev.GetDevice(), &bufferDeviceAddressInfo);

	return scratchBuffer;
}

// TODO Not needed
void PipelineSimpleRaytracing::DeleteScratchBuffer(VulkanDevice& vkDev, ScratchBuffer& scratchBuffer)
{
	if (scratchBuffer.memory_ != VK_NULL_HANDLE)
	{
		vkFreeMemory(vkDev.GetDevice(), scratchBuffer.memory_, nullptr);
	}
	if (scratchBuffer.handle_ != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(vkDev.GetDevice(), scratchBuffer.handle_, nullptr);
	}
}

uint64_t PipelineSimpleRaytracing::GetBufferDeviceAddress(VulkanDevice& vkDev, VkBuffer buffer)
{
	VkBufferDeviceAddressInfoKHR bufferDeviceAI =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = buffer
	};
	return vkGetBufferDeviceAddressKHR(vkDev.GetDevice(), &bufferDeviceAI);
}

void PipelineSimpleRaytracing::CreateShaderBindingTable(VulkanDevice& vkDev)
{
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR properties = vkDev.GetRayTracingPipelineProperties();

	const uint32_t handleSize = properties.shaderGroupHandleSize;
	const uint32_t handleSizeAligned = AlignedSize(properties.shaderGroupHandleSize, properties.shaderGroupHandleAlignment);
	const uint32_t groupCount = static_cast<uint32_t>(shaderGroups_.size());
	const uint32_t sbtSize = groupCount * handleSizeAligned;

	std::vector<uint8_t> shaderHandleStorage(sbtSize);
	VK_CHECK(vkGetRayTracingShaderGroupHandlesKHR(
		vkDev.GetDevice(), 
		pipeline_, 
		0, 
		groupCount, 
		sbtSize, 
		shaderHandleStorage.data()));

	const VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	const VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	raygenShaderBindingTable_.CreateBuffer(vkDev, handleSize, bufferUsage, memoryUsage);
	missShaderBindingTable_.CreateBuffer(vkDev, handleSize, bufferUsage, memoryUsage);
	hitShaderBindingTable_.CreateBuffer(vkDev, handleSize, bufferUsage, memoryUsage);

	// Copy handles
	raygenShaderBindingTable_.UploadBufferData(vkDev, 0, shaderHandleStorage.data(), handleSize);
	missShaderBindingTable_.UploadBufferData(vkDev, 0, shaderHandleStorage.data() + handleSizeAligned, handleSize);
	hitShaderBindingTable_.UploadBufferData(vkDev, 0, shaderHandleStorage.data() + handleSizeAligned * 2, handleSize);
}

void PipelineSimpleRaytracing::TransitionImageLayoutCommand(
	VkCommandBuffer commandBuffer,
	VkImage image,
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	uint32_t layerCount,
	uint32_t mipLevels)
{
	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = 0,
		.dstAccessMask = 0,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = VkImageSubresourceRange {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = mipLevels,
			.baseArrayLayer = 0,
			.layerCount = layerCount
		}
	};

	VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
		(format == VK_FORMAT_D16_UNORM) ||
		(format == VK_FORMAT_X8_D24_UNORM_PACK32) ||
		(format == VK_FORMAT_D32_SFLOAT) ||
		(format == VK_FORMAT_S8_UINT) ||
		(format == VK_FORMAT_D16_UNORM_S8_UINT) ||
		(format == VK_FORMAT_D24_UNORM_S8_UINT))
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_GENERAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	/* Convert back from read-only to updateable */
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	/* Convert from updateable texture to shader read-only */
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	/* Convert depth texture from undefined state to depth-stencil buffer */
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
		newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}

	/* Wait for render pass to complete */
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = 0; // VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = 0;
		/*
				sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		///		destinationStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
				destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		*/
		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	/* Convert back from read-only to color attachment */
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	/* Convert from updateable texture to shader read-only */
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	/* Convert back from read-only to depth attachment */
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	}
	/* Convert from updateable depth texture to shader read-only */
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
	{
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}

	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_GENERAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	}

	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	}

	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
		newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	}

	vkCmdPipelineBarrier(
		commandBuffer, // commandBuffer
		sourceStage, // srcStageMask
		destinationStage, // dstStageMask
		0, // dependencyFlags
		0, // memoryBarrierCount
		nullptr, // pMemoryBarriers
		0, // bufferMemoryBarrierCount
		nullptr, // pBufferMemoryBarriers
		1, // imageMemoryBarrierCount
		&barrier // pImageMemoryBarriers
	);
}