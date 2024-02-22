#ifndef VULKAN_BUFFER
#define VULKAN_BUFFER

#include "volk.h"
#include "vk_mem_alloc.h"

#include "VulkanContext.h"

class VulkanBuffer
{
public:
	VkBuffer buffer_;
	VmaAllocation vmaAllocation_;
	VmaAllocationInfo vmaInfo_;

	// Only used for raytracing
	uint64_t deviceAddress_;

	VmaAllocator vmaAllocator_;

private:
	bool isIndirectBuffer_;

public:
	VulkanBuffer() :
		buffer_(nullptr),
		vmaAllocator_(nullptr),
		vmaAllocation_(nullptr),
		deviceAddress_(0),
		vmaInfo_({})
	{
	}

	void Destroy()
	{
		if (vmaAllocation_)
		{
			vmaDestroyBuffer(vmaAllocator_, buffer_, vmaAllocation_);
		}
	}

	void CreateBuffer(
		VulkanContext& ctx,
		VkDeviceSize size,
		VkBufferUsageFlags bufferUsage,
		VmaMemoryUsage memoryUsage,
		VmaAllocationCreateFlags flags = VMA_ALLOCATION_CREATE_MAPPED_BIT); // Not sure want to keep this default value

	void CreateIndirectBuffer(
		VulkanContext& ctx,
		VkDeviceSize size);
	VkDrawIndirectCommand* MapIndirectBuffer();
	void UnmapIndirectBuffer();

	void CreateBufferWithShaderDeviceAddress(VulkanContext& ctx,
		VkDeviceSize size,
		VkBufferUsageFlags bufferUsage,
		VmaMemoryUsage memoryUsage,
		VmaAllocationCreateFlags flags = VMA_ALLOCATION_CREATE_MAPPED_BIT); // Not sure want to keep this default value

	void CreateGPUOnlyBuffer(
		VulkanContext& ctx,
		VkDeviceSize bufferSize_,
		const void* bufferData,
		VkMemoryPropertyFlags flags
	);

	void CopyFrom(
		VulkanContext& ctx,
		VkBuffer srcBuffer,
		VkDeviceSize size);

	void UploadOffsetBufferData(
		VulkanContext& ctx,
		const void* data,
		VkDeviceSize offset,
		VkDeviceSize dataSize);

	void UploadBufferData(
		VulkanContext& ctx,
		const void* data,
		const size_t dataSize);

	void DownloadBufferData(
		VulkanContext& ctx,
		void* outData,
		const size_t dataSize);
};
#endif