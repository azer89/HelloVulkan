#include "VulkanTexture.h"
#include "VulkanUtility.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_resize2.h"

void VulkanTexture::CreateTextureSampler(
	VkDevice device,
	VkSampler& sampler,
	float minLod,
	float maxLod,
	VkFilter minFilter,
	VkFilter maxFilter,
	VkSamplerAddressMode addressMode)
{
	const VkSamplerCreateInfo samplerInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = addressMode, // VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = addressMode, // VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = addressMode, // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_FALSE,
		.maxAnisotropy = 1,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = minLod,
		.maxLod = maxLod,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE
	};

	VK_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, &sampler));
}

void VulkanTexture::CreateTextureSampler(
	VkDevice device,
	float minLod, // 0.f,
	float maxLod, // 0.f,
	VkFilter minFilter,
	VkFilter maxFilter,
	VkSamplerAddressMode addressMode)
{
	CreateTextureSampler(
		device,
		sampler_,
		minLod, // 0.f,
		maxLod, // 0.f,
		minFilter,
		maxFilter,
		addressMode);
}

void VulkanTexture::CreateTextureImageViewSampler(
	VulkanDevice& vkDev,
	const char* filename)
{
	CreateTextureImage(vkDev, filename);
	image_.CreateImageView(
		vkDev.GetDevice(),
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT);
	CreateTextureSampler(vkDev.GetDevice());
}

void VulkanTexture::CreateTextureImage(
	VulkanDevice& vkDev,
	const char* filename,
	uint32_t* outTexWidth,
	uint32_t* outTexHeight)
{
	stbi_set_flip_vertically_on_load(false);

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels)
	{
		std::cerr << "Failed to load " << filename << '\n';
	}

	image_.CreateImageFromData(
		vkDev,
		pixels,
		texWidth,
		texHeight,
		NumMipMap(texWidth, texHeight),
		1, // layerCount
		VK_FORMAT_R8G8B8A8_UNORM);

	stbi_image_free(pixels);

	if (outTexWidth && outTexHeight)
	{
		*outTexWidth = (uint32_t)texWidth;
		*outTexHeight = (uint32_t)texHeight;
	}
}

void VulkanTexture::CreateHDRImage(
	VulkanDevice& vkDev,
	const char* filename)
{
	stbi_set_flip_vertically_on_load(true);

	int texWidth, texHeight, texChannels;
	float* pixels = stbi_loadf(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	image_.CreateImageFromData(
		vkDev,
		pixels,
		texWidth,
		texHeight,
		NumMipMap(texWidth, texHeight),
		1, 
		VK_FORMAT_R32G32B32A32_SFLOAT);

	stbi_image_free(pixels);
}

void VulkanTexture::Destroy(VkDevice device)
{
	image_.Destroy(device);
	vkDestroySampler(device, sampler_, nullptr);
}