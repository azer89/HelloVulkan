#ifndef VULKAN_TEXTURE
#define VULKAN_TEXTURE

#include "VulkanImage.h"
#include "VulkanDevice.h"

class VulkanTexture
{
public:
	VulkanImage image;
	VkSampler sampler;

	uint32_t width;
	uint32_t height;
	uint32_t depth;
	VkFormat format;

	// Offscreen buffers require VK_IMAGE_LAYOUT_GENERAL && static textures have VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	VkImageLayout desiredLayout;

public:
	void DestroyVulkanTexture(VkDevice device);

private:
	void DestroyVulkanImage(VkDevice device);
};

#endif
