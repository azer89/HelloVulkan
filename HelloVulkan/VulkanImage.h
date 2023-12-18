#ifndef VULKAN_IMAGE
#define VULKAN_IMAGE

#include "volk.h"

#include "VulkanDevice.h"

class VulkanImage
{
public:
	VkImage image = nullptr;
	VkDeviceMemory imageMemory = nullptr;
	VkImageView imageView = nullptr;

public:
	bool CreateDepthResources(VulkanDevice& vkDev, uint32_t width, uint32_t height);

	void Destroy(VkDevice device);

	bool CreateImageView(VkDevice device,
		VkFormat format,
		VkImageAspectFlags aspectFlags,
		VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D,
		uint32_t layerCount = 1,
		uint32_t mipLevels = 1);

	// TODO Rename to CreateImageFromData
	bool CreateTextureImageFromData(
		VulkanDevice& vkDev,
		void* imageData,
		uint32_t texWidth,
		uint32_t texHeight,
		VkFormat texFormat,
		uint32_t layerCount = 1,
		VkImageCreateFlags flags = 0);

	void CopyBufferToImage(
		VulkanDevice& vkDev,
		VkBuffer buffer,
		uint32_t width,
		uint32_t height,
		uint32_t layerCount = 1);

	bool CreateImage(
		VkDevice device, 
		VkPhysicalDevice physicalDevice, 
		uint32_t width, 
		uint32_t height, 
		VkFormat format, 
		VkImageTiling tiling, 
		VkImageUsageFlags usage, 
		VkMemoryPropertyFlags properties,  
		VkImageCreateFlags flags = 0, 
		uint32_t mipLevels = 1);

	bool UpdateTextureImage(
		VulkanDevice& vkDev,
		uint32_t texWidth,
		uint32_t texHeight,
		VkFormat texFormat,
		uint32_t layerCount,
		const void* imageData,
		VkImageLayout sourceImageLayout = VK_IMAGE_LAYOUT_UNDEFINED);

private:
	uint32_t FindMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkFormat FindDepthFormat(VkPhysicalDevice device);
	VkFormat FindSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	void TransitionImageLayout(VulkanDevice& vkDev, 
		VkFormat format, 
		VkImageLayout oldLayout, 
		VkImageLayout newLayout, 
		uint32_t layerCount = 1, 
		uint32_t mipLevels = 1);

	void TransitionImageLayoutCmd(VkCommandBuffer commandBuffer, 
		VkFormat format, 
		VkImageLayout oldLayout, 
		VkImageLayout newLayout, 
		uint32_t layerCount = 1, 
		uint32_t mipLevels = 1);

	bool HasStencilComponent(VkFormat format);

	uint32_t BytesPerTexFormat(VkFormat fmt);
};

#endif