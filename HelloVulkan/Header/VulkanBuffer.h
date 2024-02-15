#ifndef VULKAN_BUFFER
#define VULKAN_BUFFER

#include "volk.h"
#include "vk_mem_alloc.h"

#include "VulkanDevice.h"

class VulkanBuffer
{
public:
	VkBuffer buffer_ = nullptr;
	VmaAllocator vmaAllocator_ = nullptr;
	VmaAllocation vmaAllocation_ = nullptr;
	VmaAllocationInfo vmaInfo_;

	// Only used for raytracing
	uint64_t deviceAddress_ = 0;

public:
	void Destroy()
	{
		if (vmaAllocation_ == nullptr)
		{
			return;
		}

		vmaDestroyBuffer(vmaAllocator_, buffer_, vmaAllocation_);
	}

	void CreateBuffer(
		VulkanDevice& vkDev,
		VkDeviceSize size,
		VkBufferUsageFlags bufferUsage,
		VmaMemoryUsage memoryUsage,
		VmaAllocationCreateFlags flags = VMA_ALLOCATION_CREATE_MAPPED_BIT); // Not sure want to keep this default value

	void CreateBufferWithShaderDeviceAddress(VulkanDevice& vkDev,
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