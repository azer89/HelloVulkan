#include "RaytracingBuilder.h"
#include "AccelStructure.h"
#include "VulkanUtility.h"

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
	VkAccelerationStructureGeometryKHR accelerationStructureGeometry =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
		.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
		.flags = VK_GEOMETRY_OPAQUE_BIT_KHR
	};
	accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
	accelerationStructureGeometry.geometry.triangles.maxVertex = vertexCount - 1; // Highest index
	accelerationStructureGeometry.geometry.triangles.vertexStride = vertexStride;
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

	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
	accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	vkGetAccelerationStructureBuildSizesKHR(
		ctx.GetDevice(),
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&accelerationStructureBuildGeometryInfo,
		&triangleCount,
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
	scratchBuffer.CreateBufferWithShaderDeviceAddress(ctx,
		accelerationStructureBuildSizesInfo.buildScratchSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

	VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
		.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
		.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		.dstAccelerationStructure = blas->handle_,
		.geometryCount = 1,
		.pGeometries = &accelerationStructureGeometry
	};
	accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.deviceAddress_;

	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo =
	{
		.primitiveCount = triangleCount,
		.primitiveOffset = 0,
		.firstVertex = 0,
		.transformOffset = 0,
	};
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos =
	{ &accelerationStructureBuildRangeInfo };

	// Build the acceleration structure on the device via a one-time command buffer submission
	// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), 
	// but we prefer device builds
	VkCommandBuffer commandBuffer = ctx.BeginOneTimeGraphicsCommand();
	vkCmdBuildAccelerationStructuresKHR(
		commandBuffer,
		1,
		&accelerationBuildGeometryInfo,
		accelerationBuildStructureRangeInfos.data());
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
	const AccelStructure* blas,
	AccelStructure* tlas)
{
	VkAccelerationStructureInstanceKHR instance =
	{
		.transform = transformMatrix,
		.instanceCustomIndex = 0,
		.mask = 0xFF,
		.instanceShaderBindingTableRecordOffset = 0,
		.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
		.accelerationStructureReference = blas->deviceAddress_
	};

	VulkanBuffer instancesBuffer;
	instancesBuffer.CreateBufferWithShaderDeviceAddress(ctx,
		sizeof(VkAccelerationStructureInstanceKHR),
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VMA_MEMORY_USAGE_CPU_TO_GPU);
	instancesBuffer.UploadBufferData(ctx, &instance, sizeof(VkAccelerationStructureInstanceKHR));

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
		ctx.GetDevice(),
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&accelerationStructureBuildGeometryInfo,
		&primitive_count,
		&accelerationStructureBuildSizesInfo);

	tlas->Create(ctx, accelerationStructureBuildSizesInfo);

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = tlas->buffer_,
		.size = accelerationStructureBuildSizesInfo.accelerationStructureSize,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR
	};
	VK_CHECK(vkCreateAccelerationStructureKHR(
		ctx.GetDevice(), 
		&accelerationStructureCreateInfo, 
		nullptr, 
		&(tlas->handle_)));

	// Create a small scratch buffer used during build of the top level acceleration structure
	VulkanBuffer scratchBuffer;
	scratchBuffer.CreateBufferWithShaderDeviceAddress(ctx,
		accelerationStructureBuildSizesInfo.buildScratchSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY);

	VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo =
	{
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
		.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
		.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		.dstAccelerationStructure = tlas->handle_,
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
	VkCommandBuffer commandBuffer = ctx.BeginOneTimeGraphicsCommand();
	vkCmdBuildAccelerationStructuresKHR(
		commandBuffer,
		1,
		&accelerationBuildGeometryInfo,
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