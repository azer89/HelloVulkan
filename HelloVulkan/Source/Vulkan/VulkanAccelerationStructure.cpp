#include "VulkanAccelerationStructure.h"
#include "VulkanUtility.h"

void VulkanAccelerationStructure::Create(
	VulkanContext& ctx,
	VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo)
{
	VkBufferCreateInfo bufferInfo =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = buildSizeInfo.accelerationStructureSize,
		.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
	};

	VmaAllocationCreateInfo vmaAllocInfo = {
		//.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
	};

	device_ = ctx.GetDevice();
	vmaAllocator_ = ctx.GetVMAAllocator();
	VK_CHECK(vmaCreateBuffer(
		vmaAllocator_,
		&bufferInfo,
		&vmaAllocInfo,
		&buffer_,
		&vmaAllocation_,
		&vmaInfo_));
}

void VulkanAccelerationStructure::Destroy()
{
	if (vmaAllocation_)
	{
		vmaDestroyBuffer(vmaAllocator_, buffer_, vmaAllocation_);
	}

	if (handle_)
	{
		vkDestroyAccelerationStructureKHR(device_, handle_, nullptr);
	}
}