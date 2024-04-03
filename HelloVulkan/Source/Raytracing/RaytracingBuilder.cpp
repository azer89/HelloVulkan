#include "RaytracingBuilder.h"
#include "VulkanCheck.h"
#include "Utility.h"

void RaytracingBuilder::CreateRTModelDataArray(
	VulkanContext& ctx,
	Scene* scene,
	std::vector<RTModelData>& modelDataArray)
{
	uint32_t instanceCount = static_cast<uint32_t>(scene->instanceDataArray_.size());
	modelDataArray.resize(instanceCount);

	for (uint32_t i = 0; i < instanceCount; ++i)
	{
		PopulateRTModelData(
			ctx,
			scene->GetVertices(i),
			scene->GetIndices(i),
			scene->modelSSBOs_[i].model,
			&modelDataArray[i]
		);
	}
}

void RaytracingBuilder::PopulateRTModelData(
	VulkanContext& ctx,
	const std::span<VertexData> vertices,
	const std::span<uint32_t> indices,
	const glm::mat4 modelMatrix,
	RTModelData* modelData)
{
	// Vertices
	modelData->vertexCount_ = static_cast<uint32_t>(vertices.size());
	modelData->vertexBuffer_.CreateBufferWithDeviceAddress(
		ctx,
		vertices.size() * sizeof(VertexData),
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	modelData->vertexBuffer_.UploadBufferData(ctx, vertices.data(), vertices.size() * sizeof(VertexData));

	// Indices
	modelData->indexCount_ = static_cast<uint32_t>(indices.size());
	modelData->indexBuffer_.CreateBufferWithDeviceAddress(
		ctx,
		indices.size() * sizeof(uint32_t),
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	modelData->indexBuffer_.UploadBufferData(ctx, indices.data(), indices.size() * sizeof(uint32_t));

	// Model matrix
	VkTransformMatrixKHR transformMatrix{};
	auto m = glm::mat3x4(glm::transpose(modelMatrix));
	memcpy(&transformMatrix, (void*)&m, sizeof(glm::mat3x4));

	modelData->transformBuffer_.CreateBufferWithDeviceAddress(
		ctx,
		sizeof(VkTransformMatrixKHR),
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	modelData->transformBuffer_.UploadBufferData(ctx, &transformMatrix, sizeof(VkTransformMatrixKHR));
}

void RaytracingBuilder::CreateBLASMultipleMeshes(
	VulkanContext& ctx,
	const std::span<RTModelData> modelDataArray,
	AccelStructure* blas)
{
	uint32_t instanceCount = static_cast<uint32_t>(modelDataArray.size());

	std::vector<uint32_t> maxPrimitiveCounts(instanceCount, 0);
	std::vector<VkAccelerationStructureGeometryKHR> geometries(instanceCount);
	std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRangeInfos(instanceCount);
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfos(instanceCount);
	//std::vector<GeometryNode> geometryNodes{};

	for (uint32_t i = 0; i < modelDataArray.size(); ++i)
	{
		RTModelData& mData = modelDataArray[i];

		VkDeviceOrHostAddressConstKHR vAddress = {};
		VkDeviceOrHostAddressConstKHR iAddress = {};
		VkDeviceOrHostAddressConstKHR tAddress = {};

		vAddress.deviceAddress = mData.vertexBuffer_.deviceAddress_;
		iAddress.deviceAddress = mData.indexBuffer_.deviceAddress_;
		tAddress.deviceAddress = mData.transformBuffer_.deviceAddress_;

		VkAccelerationStructureGeometryKHR geometry{};
		geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		geometry.geometry.triangles.vertexData = vAddress;
		geometry.geometry.triangles.maxVertex = mData.vertexCount_;
		geometry.geometry.triangles.vertexStride = sizeof(VertexData);
		geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
		geometry.geometry.triangles.indexData = iAddress;
		geometry.geometry.triangles.transformData = tAddress;
	
		geometries[i] = geometry;
		maxPrimitiveCounts[i] = mData.indexCount_ / 3;

		VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo =
		{
			.primitiveCount = mData.indexCount_ / 3,
			.primitiveOffset = 0, // primitive->firstIndex * sizeof(uint32_t);
			.firstVertex = 0,
			.transformOffset = 0
		};
		
		buildRangeInfos[i] = buildRangeInfo;
	}

	for (uint32_t i = 0; i < modelDataArray.size(); ++i)
	{
		pBuildRangeInfos[i] = &(buildRangeInfos[i]);
	}

	VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
		.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
		.geometryCount = static_cast<uint32_t>(geometries.size()),
		.pGeometries = geometries.data()
	};

	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
	accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(
		ctx.GetDevice(),
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&accelerationStructureBuildGeometryInfo,
		maxPrimitiveCounts.data(),
		&accelerationStructureBuildSizesInfo);
	
	blas->Create(ctx, accelerationStructureBuildSizesInfo);

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = blas->buffer_,
		.size = accelerationStructureBuildSizesInfo.accelerationStructureSize,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
	};
	VK_CHECK(vkCreateAccelerationStructureKHR(ctx.GetDevice(), &accelerationStructureCreateInfo, nullptr, &(blas->handle_)));

	// Create a small scratch buffer used during build of the bottom level acceleration structure
	VulkanBuffer scratchBuffer;
	scratchBuffer.CreateBufferWithDeviceAddress(ctx,
		accelerationStructureBuildSizesInfo.buildScratchSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

	accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	accelerationStructureBuildGeometryInfo.dstAccelerationStructure = blas->handle_;
	accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress_;

	const VkAccelerationStructureBuildRangeInfoKHR* buildOffsetInfo = buildRangeInfos.data();

	// Build the acceleration structure on the device via a one-time command buffer submission
		// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
	VkCommandBuffer commandBuffer = ctx.BeginOneTimeGraphicsCommand();
	vkCmdBuildAccelerationStructuresKHR(
		commandBuffer,
		1,
		&accelerationStructureBuildGeometryInfo,
		pBuildRangeInfos.data());
	ctx.EndOneTimeGraphicsCommand(commandBuffer);

	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
		.accelerationStructure = blas->handle_
	};
	blas->deviceAddress_ = vkGetAccelerationStructureDeviceAddressKHR(ctx.GetDevice(), &accelerationDeviceAddressInfo);

	scratchBuffer.Destroy();
}

// TODO Currently can only handle a single vertex buffer
void RaytracingBuilder::CreateBLAS(VulkanContext& ctx, 
	const VulkanBuffer& vertexBuffer,
	const VulkanBuffer& indexBuffer,
	const VulkanBuffer& transformBuffer,
	uint32_t triangleCount,
	uint32_t vertexCount,
	VkDeviceSize vertexStride,
	AccelStructure* blas)
{
	VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
	VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
	VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};

	vertexBufferDeviceAddress.deviceAddress = vertexBuffer.deviceAddress_;
	indexBufferDeviceAddress.deviceAddress = indexBuffer.deviceAddress_;
	transformBufferDeviceAddress.deviceAddress = transformBuffer.deviceAddress_;

	// Build
	VkAccelerationStructureGeometryKHR accelStructureGeometry =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
		.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
		.flags = VK_GEOMETRY_OPAQUE_BIT_KHR
	};
	accelStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	accelStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	accelStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
	accelStructureGeometry.geometry.triangles.maxVertex = vertexCount; // Highest index
	accelStructureGeometry.geometry.triangles.vertexStride = vertexStride;
	accelStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	accelStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;
	accelStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
	accelStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;
	accelStructureGeometry.geometry.triangles.transformData = transformBufferDeviceAddress;

	// Get size info
	VkAccelerationStructureBuildGeometryInfoKHR accelStructureBuildGeometryInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
		.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
		.geometryCount = 1,
		.pGeometries = &accelStructureGeometry
	};

	VkAccelerationStructureBuildSizesInfoKHR accelStructureBuildSizesInfo{};
	accelStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(
		ctx.GetDevice(),
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&accelStructureBuildGeometryInfo,
		&triangleCount,
		&accelStructureBuildSizesInfo);

	blas->Create(ctx, accelStructureBuildSizesInfo);

	VkAccelerationStructureCreateInfoKHR accelStructureCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = blas->buffer_,
		.size = accelStructureBuildSizesInfo.accelerationStructureSize,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
	};
	VK_CHECK(vkCreateAccelerationStructureKHR(ctx.GetDevice(), &accelStructureCreateInfo, nullptr, &(blas->handle_)));

	// Create a small scratch buffer used during build of the bottom level acceleration structure
	VulkanBuffer scratchBuffer;
	scratchBuffer.CreateBufferWithDeviceAddress(ctx,
		accelStructureBuildSizesInfo.buildScratchSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

	VkAccelerationStructureBuildGeometryInfoKHR accelBuildGeometryInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
		.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
		.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		.dstAccelerationStructure = blas->handle_,
		.geometryCount = 1,
		.pGeometries = &accelStructureGeometry
	};
	accelBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress_;

	VkAccelerationStructureBuildRangeInfoKHR accelStructureBuildRangeInfo =
	{
		.primitiveCount = triangleCount,
		.primitiveOffset = 0,
		.firstVertex = 0,
		.transformOffset = 0,
	};
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelBuildStructureRangeInfos =
	{ &accelStructureBuildRangeInfo };

	// Build the acceleration structure on the device via a one-time command buffer submission
	// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), 
	// but we prefer device builds
	VkCommandBuffer commandBuffer = ctx.BeginOneTimeGraphicsCommand();
	vkCmdBuildAccelerationStructuresKHR(
		commandBuffer,
		1,
		&accelBuildGeometryInfo,
		accelBuildStructureRangeInfos.data());
	ctx.EndOneTimeGraphicsCommand(commandBuffer);

	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
		.accelerationStructure = blas->handle_,
	};
	blas->deviceAddress_ = vkGetAccelerationStructureDeviceAddressKHR(ctx.GetDevice(), &accelerationDeviceAddressInfo);

	scratchBuffer.Destroy();
}

void RaytracingBuilder::CreateTLAS(VulkanContext& ctx, 
	VkTransformMatrixKHR& transformMatrix,
	uint64_t blasDeviceAddress,
	AccelStructure* tlas)
{
	VkAccelerationStructureInstanceKHR instance =
	{
		.transform = transformMatrix,
		.instanceCustomIndex = 0,
		.mask = 0xFF,
		.instanceShaderBindingTableRecordOffset = 0,
		.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
		.accelerationStructureReference = blasDeviceAddress
	};

	VulkanBuffer instancesBuffer;
	instancesBuffer.CreateBufferWithDeviceAddress(ctx,
		sizeof(VkAccelerationStructureInstanceKHR),
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_CPU_TO_GPU);
	instancesBuffer.UploadBufferData(ctx, &instance, sizeof(VkAccelerationStructureInstanceKHR));

	VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress =
	{
		.deviceAddress = instancesBuffer.deviceAddress_
	};

	VkAccelerationStructureGeometryKHR accelStructureGeometry =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
		.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
		.flags = VK_GEOMETRY_OPAQUE_BIT_KHR
	};
	accelStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	accelStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
	accelStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

	// Get size info
	/*
	The pSrcAccelerationStructure, dstAccelerationStructure, and mode members of pBuildInfo are ignored.
	Any VkDeviceOrHostAddressKHR members of pBuildInfo are ignored by this command, except that
	the hostAddress member of VkAccelerationStructureGeometryTrianglesDataKHR::transformData will
	be examined to check if it is NULL.
	*/
	VkAccelerationStructureBuildGeometryInfoKHR accelStructureBuildGeometryInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
		.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
		.geometryCount = 1,
		.pGeometries = &accelStructureGeometry
	};
	uint32_t primitive_count = 1;

	VkAccelerationStructureBuildSizesInfoKHR accelStructureBuildSizesInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
	};
	vkGetAccelerationStructureBuildSizesKHR(
		ctx.GetDevice(),
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&accelStructureBuildGeometryInfo,
		&primitive_count,
		&accelStructureBuildSizesInfo);

	tlas->Create(ctx, accelStructureBuildSizesInfo);

	VkAccelerationStructureCreateInfoKHR accelStructureCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = tlas->buffer_,
		.size = accelStructureBuildSizesInfo.accelerationStructureSize,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR
	};
	VK_CHECK(vkCreateAccelerationStructureKHR(
		ctx.GetDevice(), 
		&accelStructureCreateInfo, 
		nullptr, 
		&(tlas->handle_)));

	// Create a small scratch buffer used during build of the top level acceleration structure
	VulkanBuffer scratchBuffer;
	scratchBuffer.CreateBufferWithDeviceAddress(ctx,
		accelStructureBuildSizesInfo.buildScratchSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

	VkAccelerationStructureBuildGeometryInfoKHR accelBuildGeometryInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
		.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
		.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		.dstAccelerationStructure = tlas->handle_,
		.geometryCount = 1,
		.pGeometries = &accelStructureGeometry,
	};
	accelBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress_;

	VkAccelerationStructureBuildRangeInfoKHR accelStructureBuildRangeInfo =
	{
		.primitiveCount = 1,
		.primitiveOffset = 0,
		.firstVertex = 0,
		.transformOffset = 0,
	};
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelStructureBuildRangeInfo };

	// Build the acceleration structure on the device via a one-time command buffer submission
	// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), 
	// but we prefer device builds
	VkCommandBuffer commandBuffer = ctx.BeginOneTimeGraphicsCommand();
	vkCmdBuildAccelerationStructuresKHR(
		commandBuffer,
		1,
		&accelBuildGeometryInfo,
		accelerationBuildStructureRangeInfos.data());
	ctx.EndOneTimeGraphicsCommand(commandBuffer);

	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
		.accelerationStructure = tlas->handle_
	};
	tlas->deviceAddress_ = vkGetAccelerationStructureDeviceAddressKHR(ctx.GetDevice(), &accelerationDeviceAddressInfo);

	scratchBuffer.Destroy();
	instancesBuffer.Destroy();
}

/*void RaytracingBuilder::CreateTransformBuffer(
	VulkanContext& ctx,
	const std::span<ModelUBO> uboArray,
	VulkanBuffer& transformBuffer)
{
	std::vector<VkTransformMatrixKHR> transformMatrices(uboArray.size());

	for (uint32_t i = 0; i < uboArray.size(); ++i)
	{
		VkTransformMatrixKHR transformMatrix{};
		auto m = glm::mat3x4(glm::transpose(uboArray[i].model));
		memcpy(&transformMatrix, (void*)&m, sizeof(glm::mat3x4));
		transformMatrices[i] = transformMatrix;
	}

	transformBuffer.CreateBufferWithDeviceAddress(
		ctx,
		sizeof(VkTransformMatrixKHR),
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	transformBuffer.UploadBufferData(ctx,
		transformMatrices.data(),
		static_cast<uint32_t>(transformMatrices.size()) * sizeof(VkTransformMatrixKHR));
}*/