#include "VulkanBuffer.h"
#include "VulkanUtility.h"

#include <iostream>

void VulkanBuffer::CreateBuffer(
	VulkanDevice& vkDev,
	VkDeviceSize size,
	VkBufferUsageFlags bufferUsage,
	VmaMemoryUsage memoryUsage,
	VmaAllocationCreateFlags flags)
{
	const VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = size,
		.usage = bufferUsage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr
	};

	VmaAllocationCreateInfo vmaAllocInfo = {
		.flags = flags,
		.usage = memoryUsage,
	};

	vmaAllocator_ = vkDev.GetVMAAllocator();
	VK_CHECK(vmaCreateBuffer(
		vmaAllocator_,
		&bufferInfo, 
		&vmaAllocInfo, 
		&buffer_, 
		&vmaAllocation_,
		&vmaInfo_));
}

// TODO Maybe rename to CreateGPUOnlyBuffer
void VulkanBuffer::CreateLocalMemoryBuffer
(
	VulkanDevice& vkDev,
	size_t bufferSize_,
	const void* bufferData,
	VkMemoryPropertyFlags flags
)
{
	VulkanBuffer stagingBuffer;
	stagingBuffer.CreateBuffer(
		vkDev,
		bufferSize_,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_ONLY
	);

	void* data;
	vmaMapMemory(stagingBuffer.vmaAllocator_, stagingBuffer.vmaAllocation_,  &data);
	memcpy(data, bufferData, bufferSize_);
	vmaUnmapMemory(stagingBuffer.vmaAllocator_, stagingBuffer.vmaAllocation_);

	CreateBuffer(
		vkDev,
		bufferSize_,
		flags,
		VMA_MEMORY_USAGE_GPU_ONLY);
	CopyFrom(vkDev, stagingBuffer.buffer_, bufferSize_);

	stagingBuffer.Destroy();
}

void VulkanBuffer::CopyFrom(VulkanDevice& vkDev, VkBuffer srcBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = vkDev.BeginOneTimeGraphicsCommand();

	const VkBufferCopy copyRegion = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size
	};

	vkCmdCopyBuffer(commandBuffer, srcBuffer, buffer_, 1, &copyRegion);

	vkDev.EndOneTimeGraphicsCommand(commandBuffer);
}

void VulkanBuffer::UploadBufferData(
	VulkanDevice& vkDev,
	VkDeviceSize deviceOffset,
	const void* data,
	const size_t dataSize)
{
	void* mappedData = nullptr;
	vmaMapMemory(vmaAllocator_, vmaAllocation_, &mappedData);
	memcpy(mappedData, data, dataSize);
	vmaUnmapMemory(vmaAllocator_, vmaAllocation_);
}

void VulkanBuffer::DownloadBufferData(VulkanDevice& vkDev,
	VkDeviceSize deviceOffset,
	void* outData,
	const size_t dataSize)
{
	void* mappedData = nullptr;
	vmaMapMemory(vmaAllocator_, vmaAllocation_, &mappedData);
	memcpy(outData, mappedData, dataSize);
	vmaUnmapMemory(vmaAllocator_, vmaAllocation_);
}