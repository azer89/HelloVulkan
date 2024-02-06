#include "VulkanBuffer.h"
#include "VulkanUtility.h"

#include <iostream>

void VulkanBuffer::CreateBuffer(
	VulkanDevice& vkDev,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VmaMemoryUsage memoryUsage,
	VmaAllocationCreateFlags flags)
{
	const VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr
	};

	// VMA_ALLOCATION_CREATE_MAPPED_BIT
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

	std::cout << "create buffer\n";

	/*VK_CHECK(vkCreateBuffer(vkDev.GetDevice(), &bufferInfo, nullptr, &buffer_));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(vkDev.GetDevice(), buffer_, &memRequirements);

	const VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = FindMemoryType(vkDev.GetPhysicalDevice(), memRequirements.memoryTypeBits, properties)
	};

	VK_CHECK(vkAllocateMemory(vkDev.GetDevice(), &allocInfo, nullptr, &bufferMemory_));

	vkBindBufferMemory(vkDev.GetDevice(), buffer_, bufferMemory_, 0);*/
}

// Buffer with memory property of VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
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

	/*
	usageFlagBits can be
		vertex buffer --> VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
		index buffer --> VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
		bindless buffer --> VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
	*/
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

uint32_t VulkanBuffer::FindMemoryType(
	VkPhysicalDevice device,
	uint32_t typeFilter,
	VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(device, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	return 0xFFFFFFFF;
}