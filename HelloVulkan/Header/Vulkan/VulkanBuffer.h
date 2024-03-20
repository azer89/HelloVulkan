#ifndef VULKAN_BUFFER
#define VULKAN_BUFFER

#include "VulkanContext.h"

#include "vk_mem_alloc.h"

class VulkanBuffer
{
public:
	VkBuffer buffer_;
	VmaAllocation vmaAllocation_;
	VmaAllocationInfo vmaInfo_;
	VkDeviceSize size_;

	// Only used for raytracing
	uint64_t deviceAddress_;

	VmaAllocator vmaAllocator_;

public:
	VulkanBuffer() :
		buffer_(nullptr),
		vmaAllocation_(nullptr),
		vmaInfo_({}),
		deviceAddress_(0),
		vmaAllocator_(nullptr)
	{
	}

	void Destroy()
	{
		if (vmaAllocation_)
		{
			vmaDestroyBuffer(vmaAllocator_, buffer_, vmaAllocation_);
			buffer_ = nullptr;
			vmaAllocation_ = nullptr;
		}
	}

	void CreateBuffer(
		VulkanContext& ctx,
		VkDeviceSize size,
		VkBufferUsageFlags bufferUsage,
		VmaMemoryUsage memoryUsage,
		VmaAllocationCreateFlags flags = VMA_ALLOCATION_CREATE_MAPPED_BIT); // Not sure want to keep this default value

	void CreateGPUOnlyIndirectBuffer(
		VulkanContext& ctx,
		const void* bufferData,
		VkDeviceSize size);
	void CreateMappedIndirectBuffer(
		VulkanContext& ctx,
		VkDeviceSize size);
	VkDrawIndirectCommand* MapIndirectBuffer();
	void UnmapIndirectBuffer();

	void CreateBufferWithShaderDeviceAddress(
		VulkanContext& ctx,
		VkDeviceSize size,
		VkBufferUsageFlags bufferUsage,
		VmaMemoryUsage memoryUsage,
		VmaAllocationCreateFlags flags = VMA_ALLOCATION_CREATE_MAPPED_BIT); // Not sure want to keep this default value

	void CreateGPUOnlyBuffer(
		VulkanContext& ctx,
		VkDeviceSize bufferSize_,
		const void* bufferData,
		VkBufferUsageFlags bufferUsage
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

	VkDescriptorBufferInfo GetBufferInfo() const
	{
		return
		{
			.buffer = buffer_,
			.offset = 0,
			.range = size_
		};
	}

	// Helper function
	static void CreateMultipleUniformBuffers(
		VulkanContext& ctx,
		std::vector<VulkanBuffer>& buffers,
		uint32_t dataSize,
		size_t bufferCount);
};
#endif