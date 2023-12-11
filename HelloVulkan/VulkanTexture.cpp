#include "VulkanTexture.h"

void VulkanTexture::DestroyVulkanTexture(VkDevice device)
{
	DestroyVulkanImage(device);
	vkDestroySampler(device, sampler, nullptr);
}


void VulkanTexture::DestroyVulkanImage(VkDevice device)
{
	vkDestroyImageView(device, image.imageView, nullptr);
	vkDestroyImage(device, image.image, nullptr);
	vkFreeMemory(device, image.imageMemory, nullptr);
}