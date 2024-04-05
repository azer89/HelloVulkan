#include "VulkanBuffer.h"
#include "VulkanCheck.h"

#include <iostream>

void VulkanBuffer::CreateBuffer(
	VulkanContext& ctx,
	VkDeviceSize size,
	VkBufferUsageFlags bufferUsage,
	VmaMemoryUsage memoryUsage,
	VmaAllocationCreateFlags flags)
{
	size_ = size;
	
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

	if (bufferUsage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
	{
		VkBufferDeviceAddressInfoKHR bufferDeviceAddressInfo =
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
			.buffer = buffer_
		};

		deviceAddress_ = vkGetBufferDeviceAddressKHR(ctx.GetDevice(), &bufferDeviceAddressInfo);
	}
}

void VulkanBuffer::CreateGPUOnlyIndirectBuffer(
		VulkanContext& ctx,
		const void* bufferData,
		VkDeviceSize size)
{
	CreateGPUOnlyBuffer(
		ctx,
		size,
		bufferData,
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
	);
}

void VulkanBuffer::CreateMappedIndirectBuffer(
	VulkanContext& ctx,
	VkDeviceSize size)
{
	CreateBuffer(ctx,
		size,
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU, // TODO Is it possible to be GPU only?
		VMA_ALLOCATION_CREATE_MAPPED_BIT);
}

VkDrawIndirectCommand* VulkanBuffer::MapIndirectBuffer()
{
	VkDrawIndirectCommand* mappedData = nullptr;
	vmaMapMemory(vmaAllocator_, vmaAllocation_, (void**)&mappedData);
	return mappedData;
}

void VulkanBuffer::UnmapIndirectBuffer()
{
	vmaUnmapMemory(vmaAllocator_, vmaAllocation_);
}

void VulkanBuffer::CreateBufferWithDeviceAddress(VulkanContext& ctx,
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
}

void VulkanBuffer::CreateGPUOnlyBuffer
(
	VulkanContext& ctx,
	VkDeviceSize bufferSize_,
	const void* bufferData,
	VkBufferUsageFlags bufferUsage
)
{
	VulkanBuffer stagingBuffer;
	stagingBuffer.CreateBuffer(
		ctx,
		bufferSize_,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_ONLY // TODO Deprecated flag
	);

	void* data;
	vmaMapMemory(stagingBuffer.vmaAllocator_, stagingBuffer.vmaAllocation_,  &data);
	memcpy(data, bufferData, bufferSize_);
	vmaUnmapMemory(stagingBuffer.vmaAllocator_, stagingBuffer.vmaAllocation_);

	CreateBuffer(
		ctx,
		bufferSize_,
		bufferUsage,
		VMA_MEMORY_USAGE_GPU_ONLY, // TODO Deprecated flag
		0);
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

void VulkanBuffer::CreateMultipleUniformBuffers(
	VulkanContext& ctx,
	std::vector<VulkanBuffer>& buffers,
	uint32_t dataSize,
	size_t bufferCount)
{
	buffers.resize(bufferCount);
	for (size_t i = 0; i < bufferCount; i++)
	{
		buffers[i].CreateBuffer(
			ctx,
			dataSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU
		);
	}
}