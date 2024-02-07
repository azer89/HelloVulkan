#ifndef VULKAN_BUFFER
#define VULKAN_BUFFER

#include "volk.h"
#include "vk_mem_alloc.h"

#include "VulkanDevice.h"

class VulkanBuffer
{
public:
	VkBuffer buffer_;
	VmaAllocator vmaAllocator_;
	VmaAllocation vmaAllocation_;
	VmaAllocationInfo vmaInfo_;

public:
	void Destroy()
	{
		vmaDestroyBuffer(vmaAllocator_, buffer_, vmaAllocation_);
	}

	void CreateBuffer(
		VulkanDevice& vkDev,
		VkDeviceSize size,
		VkBufferUsageFlags bufferUsage,
		VmaMemoryUsage memoryUsage,
		VmaAllocationCreateFlags flags = VMA_ALLOCATION_CREATE_MAPPED_BIT); // Not sure want to keep this default value

	void CreateGPUOnlyBuffer(
		VulkanDevice& vkDev,
		size_t bufferSize_,
		const void* bufferData,
		VkMemoryPropertyFlags flags
	);

	void CopyFrom(
		VulkanDevice& vkDev,
		VkBuffer srcBuffer,
		VkDeviceSize size);

	void UploadBufferData(
		VulkanDevice& vkDev,
		VkDeviceSize deviceOffset,
		const void* data,
		const size_t dataSize);

	void DownloadBufferData(
		VulkanDevice& vkDev,
		VkDeviceSize deviceOffset,
		void* outData,
		const size_t dataSize);
};
#endif