#include "VulkanBuffer.h"
#include "VulkanUtility.h"

#include <iostream>

void VulkanBuffer::CreateBuffer(
	VulkanContext& ctx,
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

	vmaAllocator_ = ctx.GetVMAAllocator();
	VK_CHECK(vmaCreateBuffer(
		vmaAllocator_,
		&bufferInfo, 
		&vmaAllocInfo, 
		&buffer_, 
		&vmaAllocation_,
		&vmaInfo_));
}

void VulkanBuffer::CreateBufferWithShaderDeviceAddress(VulkanContext& ctx,
	VkDeviceSize size,
	VkBufferUsageFlags bufferUsage,
	VmaMemoryUsage memoryUsage,
	VmaAllocationCreateFlags flags)
{
	CreateBuffer(ctx,
		size,
		bufferUsage | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		memoryUsage,
		flags);

	VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = buffer_
	};

	deviceAddress_ = vkGetBufferDeviceAddressKHR(ctx.GetDevice(), &bufferDeviceAddressInfo);
}

void VulkanBuffer::CreateGPUOnlyBuffer
(
	VulkanContext& ctx,
	size_t bufferSize_,
	const void* bufferData,
	VkMemoryPropertyFlags flags
)
{
	VulkanBuffer stagingBuffer;
	stagingBuffer.CreateBuffer(
		ctx,
		bufferSize_,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_ONLY
	);

	void* data;
	vmaMapMemory(stagingBuffer.vmaAllocator_, stagingBuffer.vmaAllocation_,  &data);
	memcpy(data, bufferData, bufferSize_);
	vmaUnmapMemory(stagingBuffer.vmaAllocator_, stagingBuffer.vmaAllocation_);

	CreateBuffer(
		ctx,
		bufferSize_,
		flags,
		VMA_MEMORY_USAGE_GPU_ONLY);
	CopyFrom(ctx, stagingBuffer.buffer_, bufferSize_);

	stagingBuffer.Destroy();
}

void VulkanBuffer::CopyFrom(VulkanContext& ctx, VkBuffer srcBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = ctx.BeginOneTimeGraphicsCommand();

	const VkBufferCopy copyRegion = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size
	};

	vkCmdCopyBuffer(commandBuffer, srcBuffer, buffer_, 1, &copyRegion);

	ctx.EndOneTimeGraphicsCommand(commandBuffer);
}

void VulkanBuffer::UploadOffsetBufferData(
	VulkanContext& ctx,
	const void* data,
	VkDeviceSize offset,
	VkDeviceSize dataSize)
{
	vmaCopyMemoryToAllocation(vmaAllocator_, data, vmaAllocation_, offset, dataSize);
}

void VulkanBuffer::UploadBufferData(
	VulkanContext& ctx,
	const void* data,
	const size_t dataSize)
{
	/*void* mappedData = nullptr;
	vmaMapMemory(vmaAllocator_, vmaAllocation_, &mappedData);
	memcpy(mappedData, data, dataSize);
	vmaUnmapMemory(vmaAllocator_, vmaAllocation_);*/
	vmaCopyMemoryToAllocation(vmaAllocator_, data, vmaAllocation_, 0, dataSize);
}

void VulkanBuffer::DownloadBufferData(
	VulkanContext& ctx,
	void* outData,
	const size_t dataSize)
{
	void* mappedData = nullptr;
	vmaMapMemory(vmaAllocator_, vmaAllocation_, &mappedData);
	memcpy(outData, mappedData, dataSize);
	vmaUnmapMemory(vmaAllocator_, vmaAllocation_);
}