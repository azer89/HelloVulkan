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

	bool CreateBuffer(
		VkDevice device,
		VkPhysicalDevice physicalDevice,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties);

	void CopyFrom(
		VulkanDevice& vkDev,
		VkBuffer srcBuffer,
		VkDeviceSize size);

	void UploadBufferData(
		VulkanDevice& vkDev,
		VkDeviceSize deviceOffset,
		const void* data,
		const size_t dataSize);

	void DownloadBufferData(VulkanDevice& vkDev,
		VkDeviceSize deviceOffset,
		void* outData,
		const size_t dataSize);

	void CreateSharedBuffer(
		VulkanDevice& vkDev,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties);

private:
	// TODO Possibly move this to VulkanDevice
	uint32_t FindMemoryType(
		VkPhysicalDevice device,
		uint32_t typeFilter,
		VkMemoryPropertyFlags properties);
};
#endif