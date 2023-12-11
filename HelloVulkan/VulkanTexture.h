#ifndef VULKAN_TEXTURE
#define VULKAN_TEXTURE

#include "VulkanImage.h"
#include "VulkanDevice.h"

class VulkanTexture
{
public:
	void Create(VulkanDevice& vkDev, const char* fileName);

	void Destroy(VkDevice device)
	{
		image.Destroy(device);
		vkDestroySampler(device, sampler, nullptr);
	}

private:
	bool CreateTextureImage(VulkanDevice& vkDev,
		const char* filename,
		//VkImage& textureImage, 
		//VkDeviceMemory& textureImageMemory, 
		uint32_t* outTexWidth = nullptr,
		uint32_t* outTexHeight = nullptr);
	bool CreateTextureSampler(VkDevice device,
		//VkSampler* sampler, 
		VkFilter minFilter = VK_FILTER_LINEAR,
		VkFilter maxFilter = VK_FILTER_LINEAR,
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

private:
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	VkFormat format;

	VulkanImage image;
	VkSampler sampler;

	// Offscreen buffers require VK_IMAGE_LAYOUT_GENERAL && static textures have VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	VkImageLayout desiredLayout;
};

#endif
