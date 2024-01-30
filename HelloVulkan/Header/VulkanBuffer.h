#ifndef VULKAN_BUFFER
#define VULKAN_BUFFER

#include "volk.h"

#include "VulkanDevice.h"

class VulkanBuffer
{
public:
	VkBuffer buffer_;
	VkDeviceMemory bufferMemory_;

public:
	void Destroy(VkDevice device)
	{
		vkDestroyBuffer(device, buffer_, nullptr);
		vkFreeMemory(device, bufferMemory_, nullptr);
	}

	void CreateBuffer(
		VulkanDevice& vkDev,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties);

	void CreateSharedBuffer(
		VulkanDevice& vkDev,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties);

	// Buffer with memory property of VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
	void CreateLocalMemoryBuffer
	(
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

private:
	// TODO Possibly move this to VulkanDevice
	uint32_t FindMemoryType(
		VkPhysicalDevice device,
		uint32_t typeFilter,
		VkMemoryPropertyFlags properties);
};
#endif