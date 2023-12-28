#ifndef VULKAN_TEXTURE
#define VULKAN_TEXTURE

#include "VulkanImage.h"
#include "VulkanDevice.h"

class VulkanTexture
{
public:
	VulkanImage image_;
	VkSampler sampler_;

	void CreateTextureImageViewSampler(
		VulkanDevice& vkDev,
		const char* filename);

	void CreateTextureImage(
		VulkanDevice& vkDev,
		const char* filename,
		uint32_t* outTexWidth = nullptr,
		uint32_t* outTexHeight = nullptr);

	void CreateHDRImage(
		VulkanDevice& vkDev,
		const char* filename);

	void CreateTextureSampler(
		VkDevice device,
		VkSampler& sampler,
		float minLod = 0.f,
		float maxLod = 0.f,
		VkFilter minFilter = VK_FILTER_LINEAR,
		VkFilter maxFilter = VK_FILTER_LINEAR,
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

	void CreateTextureSampler(
		VkDevice device,
		float minLod = 0.f,
		float maxLod = 0.f,
		VkFilter minFilter = VK_FILTER_LINEAR,
		VkFilter maxFilter = VK_FILTER_LINEAR,
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

	void Destroy(VkDevice device);
};

#endif
