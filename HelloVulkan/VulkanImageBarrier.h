#ifndef VULKAN_IMAGE_BARRIER
#define VULKAN_IMAGE_BARRIER

#include "VulkanImage.h"

#include "volk.h"

struct VulkanImageBarrier
{
	VulkanImageBarrier(
		const VulkanImage& image, 
		VkAccessFlags srcAccessMask, 
		VkAccessFlags dstAccessMask, 
		VkImageLayout oldLayout, 
		VkImageLayout newLayout)
	{
		barrier.srcAccessMask = srcAccessMask;
		barrier.dstAccessMask = dstAccessMask;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image.image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	}
	operator VkImageMemoryBarrier() const { return barrier; }
	VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };

	VulkanImageBarrier& aspectMask(VkImageAspectFlags aspectMask)
	{
		barrier.subresourceRange.aspectMask = aspectMask;
		return *this;
	}

	VulkanImageBarrier& mipLevels(uint32_t baseMipLevel, uint32_t levelCount = VK_REMAINING_MIP_LEVELS)
	{
		barrier.subresourceRange.baseMipLevel = baseMipLevel;
		barrier.subresourceRange.levelCount = levelCount;
		return *this;
	}

	VulkanImageBarrier& arrayLayers(uint32_t baseArrayLayer, uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS)
	{
		barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
		barrier.subresourceRange.layerCount = layerCount;
		return *this;
	}
};

#endif
