#ifndef VULKAN_TEXTURE
#define VULKAN_TEXTURE

#include "VulkanImage.h"
#include "VulkanDevice.h"
#include "Bitmap.h"

class VulkanTexture
{
public:
	VulkanImage image_;
	VkSampler sampler_;

	uint32_t width_;
	uint32_t height_;
	uint32_t layers_;
	uint32_t mipmapLevels_;
	uint32_t bindIndex_;
	VkFormat format_;

	// Offscreen buffers require VK_IMAGE_LAYOUT_GENERAL && static textures have VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	VkImageLayout desiredLayout_;

	void CreateTexture(
		VulkanDevice& vkDev,
		uint32_t width,
		uint32_t height,
		uint32_t layers,
		VkFormat format,
		uint32_t levels,
		VkImageUsageFlags additionalUsage);

	bool CreateHDRImage(
		VulkanDevice& vkDev,
		const char* filename);
	
	bool CreateTextureImage(
		VulkanDevice& vkDev,
		const char* filename,
		uint32_t* outTexWidth = nullptr,
		uint32_t* outTexHeight = nullptr);

	bool CreateCubeTextureImage(
		VulkanDevice& vkDev,
		const char* filename,
		uint32_t* width = nullptr,
		uint32_t* height = nullptr);

	bool CreateTextureSampler(
		VkDevice device,
		VkFilter minFilter = VK_FILTER_LINEAR,
		VkFilter maxFilter = VK_FILTER_LINEAR,
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

	void DestroyVulkanTexture(VkDevice device);

private:
	Bitmap ConvertEquirectangularMapToVerticalCross(const Bitmap& b);

	Bitmap ConvertVerticalCrossToCubeMapFaces(const Bitmap& b);

	void DestroyVulkanImage(VkDevice device);
};

#endif
