#include "PipelineSimpleRaytracing.h"
#include "VulkanShader.h"
#include "VulkanUtility.h"
#include "Configs.h"

PipelineSimpleRaytracing::PipelineSimpleRaytracing(VulkanDevice& vkDev) :
	PipelineBase(
		vkDev,
		{
			.type_ = PipelineType::GraphicsOnScreen
		})
{
	CreateMultipleUniformBuffers(vkDev, cameraUBOBuffers_, sizeof(RaytracingCameraUBO), AppConfig::FrameOverlapCount);

	CreateBLAS(vkDev);
	CreateTLAS(vkDev);

	CreateStorageImage(vkDev);
	CreateDescriptor(vkDev);
	CreateRayTracingPipeline(vkDev);
	CreateShaderBindingTable(vkDev);
}

PipelineSimpleRaytracing::~PipelineSimpleRaytracing()
{
	storageImage_.Destroy();
	blas_.Destroy();
	tlas_.Destroy();
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

	const uint32_t handleSizeAligned = Utility::AlignedSize(properties.shaderGroupHandleSize, properties.shaderGroupHandleAlignment);

	VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry =
	{
		.deviceAddress = raygenShaderBindingTable_.deviceAddress_,
		.stride = handleSizeAligned,
		.size = handleSizeAligned
	};

	VkStridedDeviceAddressRegionKHR missShaderSbtEntry =
	{
		.deviceAddress = missShaderBindingTable_.deviceAddress_,
		.stride = handleSizeAligned,
		.size = handleSizeAligned,
	};

	VkStridedDeviceAddressRegionKHR hitShaderSbtEntry =
	{
		.deviceAddress = hitShaderBindingTable_.deviceAddress_,
		.stride = handleSizeAligned,
		.size = handleSizeAligned
	};
	VkStridedDeviceAddressRegionKHR callableShaderSbtEntry{};

	uint32_t frameIndex = vkDev.GetFrameIndex();
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline_);
	vkCmdBindDescriptorSets(
		commandBuffer, 
		VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, 
		pipelineLayout_, 
		0, 
		1, 
		&descriptorSets_[frameIndex], 0, 0);

	vkCmdTraceRaysKHR(
		commandBuffer,
		&raygenShaderSbtEntry,
		&missShaderSbtEntry,
		&hitShaderSbtEntry,
		&callableShaderSbtEntry,
		vkDev.GetFrameBufferWidth(),
		vkDev.GetFrameBufferHeight(),
		1);

	uint32_t swapchainIndex = vkDev.GetCurrentSwapchainImageIndex();

	VulkanImage::TransitionImageLayoutCommand(
		commandBuffer,
		storageImage_.image_,
		storageImage_.imageFormat_,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		1u,
		1u);

	VulkanImage::TransitionImageLayoutCommand(
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
	
	VulkanImage::TransitionImageLayoutCommand(
		commandBuffer,
		vkDev.GetSwapchainImage(swapchainIndex),
		vkDev.GetSwapchainImageFormat(),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		1u,
		1u);

	VulkanImage::TransitionImageLayoutCommand(commandBuffer,
		storageImage_.image_,
		storageImage_.imageFormat_,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL);
}

void PipelineSimpleRaytracing::OnWindowResized(VulkanDevice& vkDev)
{
	storageImage_.Destroy();
	CreateStorageImage(vkDev);
	UpdateDescriptor(vkDev);
}

void PipelineSimpleRaytracing::CreateDescriptor(VulkanDevice& vkDev)
{
	// Pool
	descriptor_.CreatePool(
		vkDev,
		{
			.uboCount_ = 1u,
			.storageImageCount_ = 1u,
			.accelerationStructureCount_ = 1u,
			.frameCount_ = AppConfig::FrameOverlapCount,
			.setCountPerFrame_ = 1u,
		});

	// Layout 
	descriptor_.CreateLayout(vkDev,
	{
		{
			.descriptorType_ = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
			.shaderFlags_ = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			.bindingCount_ = 1
		},
		{
			.descriptorType_ = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.shaderFlags_ = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			.bindingCount_ = 1
		},
		{
			.descriptorType_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			.bindingCount_ = 1
		}
	});

	// Allocate descriptor sets
	auto frameCount = AppConfig::FrameOverlapCount;
	for (size_t i = 0; i < frameCount; i++)
	{
		descriptor_.AllocateSet(vkDev, &(descriptorSets_[i]));
	}

	// Set up descriptor sets
	UpdateDescriptor(vkDev);
}

void PipelineSimpleRaytracing::UpdateDescriptor(VulkanDevice& vkDev)
{
	// Sets
	auto frameCount = AppConfig::FrameOverlapCount;

	VkWriteDescriptorSetAccelerationStructureKHR asInfo =
	{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
		.accelerationStructureCount = 1u,
		.pAccelerationStructures = &tlas_.handle_,
	};

	VkDescriptorImageInfo imageInfo =
	{
		.imageView = storageImage_.imageView_,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};

	for (size_t i = 0; i < frameCount; i++)
	{
		VkDescriptorBufferInfo bufferInfo = { cameraUBOBuffers_[i].buffer_, 0, sizeof(RaytracingCameraUBO) };

		descriptor_.UpdateSet(
			vkDev,
			{
				{.pNext_ = &asInfo, .type_ = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR },
				{.imageInfoPtr_ = &imageInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE },
				{.bufferInfoPtr_ = &bufferInfo, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }
			},
			&(descriptorSets_[i]));
	}
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
		.pSetLayouts = &descriptor_.layout_
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
		s.Destroy();
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
		{ {  1.0f, -1.0f, 0.0f } },
		{ { -1.0f, -1.0f, 0.0f } },
		{ {  0.0f,  1.0f, 0.0f } }
	};

	// Setup indices
	std::vector<uint32_t> indices = { 0, 1, 2 };

	// Setup identity transform matrix
	VkTransformMatrixKHR transformMatrix = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f
	};

	vertexBuffer_.CreateBufferWithShaderDeviceAddress(
		vkDev,
		vertices.size() * sizeof(Vertex),
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	vertexBuffer_.UploadBufferData(vkDev, 0, vertices.data(), vertices.size() * sizeof(Vertex));

	indexBuffer_.CreateBufferWithShaderDeviceAddress(
		vkDev,
		indices.size() * sizeof(uint32_t),
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	indexBuffer_.UploadBufferData(vkDev, 0, indices.data(), indices.size() * sizeof(uint32_t));

	transformBuffer_.CreateBufferWithShaderDeviceAddress(
		vkDev,
		sizeof(VkTransformMatrixKHR),
	VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	transformBuffer_.UploadBufferData(vkDev, 0, &transformMatrix, sizeof(VkTransformMatrixKHR));

	VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
	VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
	VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};

	vertexBufferDeviceAddress.deviceAddress = vertexBuffer_.deviceAddress_;
	indexBufferDeviceAddress.deviceAddress = indexBuffer_.deviceAddress_;
	transformBufferDeviceAddress.deviceAddress = transformBuffer_.deviceAddress_;

	// Build
	VkAccelerationStructureGeometryKHR accelerationStructureGeometry =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
		.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
		.flags = VK_GEOMETRY_OPAQUE_BIT_KHR
	};
	accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
	accelerationStructureGeometry.geometry.triangles.maxVertex = 2; // Highest index
	accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(Vertex);
	accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;
	accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
	accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;
	accelerationStructureGeometry.geometry.triangles.transformData = transformBufferDeviceAddress;

	// Get size info
	VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
		.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
		.geometryCount = 1,
		.pGeometries = &accelerationStructureGeometry
	};

	const uint32_t numTriangles = 1;
	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
	accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(
		vkDev.GetDevice(),
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&accelerationStructureBuildGeometryInfo,
		&numTriangles,
		&accelerationStructureBuildSizesInfo);

	blas_.Create(vkDev, accelerationStructureBuildSizesInfo);

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = blas_.buffer_,
		.size = accelerationStructureBuildSizesInfo.accelerationStructureSize,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
	};
	vkCreateAccelerationStructureKHR(vkDev.GetDevice(), &accelerationStructureCreateInfo, nullptr, &blas_.handle_);

	// Create a small scratch buffer used during build of the bottom level acceleration structure
	VulkanBuffer scratchBuffer;
	scratchBuffer.CreateBufferWithShaderDeviceAddress(vkDev,
		accelerationStructureBuildSizesInfo.buildScratchSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

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

	scratchBuffer.Destroy();
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
	instancesBuffer.CreateBufferWithShaderDeviceAddress(vkDev,
		sizeof(VkAccelerationStructureInstanceKHR),
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_CPU_TO_GPU);
	instancesBuffer.UploadBufferData(vkDev, 0, &instance, sizeof(VkAccelerationStructureInstanceKHR));

	VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress =
	{
		.deviceAddress = instancesBuffer.deviceAddress_
	};

	VkAccelerationStructureGeometryKHR accelerationStructureGeometry =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
		.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
		.flags = VK_GEOMETRY_OPAQUE_BIT_KHR
	};
	accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
	accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

	// Get size info
	/*
	The pSrcAccelerationStructure, dstAccelerationStructure, and mode members of pBuildInfo are ignored. 
	Any VkDeviceOrHostAddressKHR members of pBuildInfo are ignored by this command, except that 
	the hostAddress member of VkAccelerationStructureGeometryTrianglesDataKHR::transformData will 
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

	tlas_.Create(vkDev, accelerationStructureBuildSizesInfo);

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = tlas_.buffer_,
		.size = accelerationStructureBuildSizesInfo.accelerationStructureSize,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR
	};
	vkCreateAccelerationStructureKHR(vkDev.GetDevice(), &accelerationStructureCreateInfo, nullptr, &tlas_.handle_);

	// Create a small scratch buffer used during build of the top level acceleration structure
	VulkanBuffer scratchBuffer;
	scratchBuffer.CreateBufferWithShaderDeviceAddress(vkDev,
		accelerationStructureBuildSizesInfo.buildScratchSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

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

	scratchBuffer.Destroy();
	instancesBuffer.Destroy();
}

void PipelineSimpleRaytracing::CreateShaderBindingTable(VulkanDevice& vkDev)
{
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR properties = vkDev.GetRayTracingPipelineProperties();

	const uint32_t handleSize = properties.shaderGroupHandleSize;
	const uint32_t handleSizeAligned = Utility::AlignedSize(properties.shaderGroupHandleSize, properties.shaderGroupHandleAlignment);
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

	const VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
	const VmaMemoryUsage memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
	raygenShaderBindingTable_.CreateBufferWithShaderDeviceAddress(vkDev, handleSize, bufferUsage, memoryUsage);
	missShaderBindingTable_.CreateBufferWithShaderDeviceAddress(vkDev, handleSize, bufferUsage, memoryUsage);
	hitShaderBindingTable_.CreateBufferWithShaderDeviceAddress(vkDev, handleSize, bufferUsage, memoryUsage);

	// Copy handles
	raygenShaderBindingTable_.UploadBufferData(vkDev, 0, shaderHandleStorage.data(), handleSize);
	missShaderBindingTable_.UploadBufferData(vkDev, 0, shaderHandleStorage.data() + handleSizeAligned, handleSize);
	hitShaderBindingTable_.UploadBufferData(vkDev, 0, shaderHandleStorage.data() + handleSizeAligned * 2, handleSize);
}