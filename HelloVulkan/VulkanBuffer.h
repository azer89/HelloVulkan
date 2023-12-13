#ifndef VULKAN_BUFFER
#define VULKAN_BUFFER

#define VK_NO_PROTOTYPES
#include "volk.h"

#include "VulkanDevice.h"
#include "VulkanUtility.h"

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

	bool CreateBuffer(
		VkDevice device,
		VkPhysicalDevice physicalDevice,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties);

	void UploadBufferData(
		VulkanDevice& vkDev,
		VkDeviceSize deviceOffset,
		const void* data,
		const size_t dataSize);

private:
	uint32_t FindMemoryType(
		VkPhysicalDevice device,
		uint32_t typeFilter,
		VkMemoryPropertyFlags properties);
};
#endif