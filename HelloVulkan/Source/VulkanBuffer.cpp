#include "VulkanBuffer.h"
#include "VulkanUtility.h"

void VulkanBuffer::CreateBuffer(
	VulkanDevice& vkDev,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties)
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

	VK_CHECK(vkCreateBuffer(vkDev.GetDevice(), &bufferInfo, nullptr, &buffer_));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(vkDev.GetDevice(), buffer_, &memRequirements);

	const VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = FindMemoryType(vkDev.GetPhysicalDevice(), memRequirements.memoryTypeBits, properties)
	};

	VK_CHECK(vkAllocateMemory(vkDev.GetDevice(), &allocInfo, nullptr, &bufferMemory_));

	vkBindBufferMemory(vkDev.GetDevice(), buffer_, bufferMemory_, 0);
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
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	void* data;
	vkMapMemory(vkDev.GetDevice(), stagingBuffer.bufferMemory_, 0, bufferSize_, 0, &data);
	memcpy(data, bufferData, bufferSize_);
	vkUnmapMemory(vkDev.GetDevice(), stagingBuffer.bufferMemory_);

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
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	CopyFrom(vkDev, stagingBuffer.buffer_, bufferSize_);

	stagingBuffer.Destroy(vkDev.GetDevice());
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
	vkMapMemory(vkDev.GetDevice(), bufferMemory_, deviceOffset, dataSize, 0, &mappedData);
	memcpy(mappedData, data, dataSize);
	vkUnmapMemory(vkDev.GetDevice(), bufferMemory_);
}

void VulkanBuffer::DownloadBufferData(VulkanDevice& vkDev,
	VkDeviceSize deviceOffset,
	void* outData,
	const size_t dataSize)
{
	void* mappedData = nullptr;
	vkMapMemory(vkDev.GetDevice(), bufferMemory_, deviceOffset, dataSize, 0, &mappedData);
	memcpy(outData, mappedData, dataSize);
	vkUnmapMemory(vkDev.GetDevice(), bufferMemory_);
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