#include "VulkanTexture.h"

void VulkanTexture::Create(VulkanDevice& vkDev, const char* fileName)
{

}

bool VulkanTexture::CreateTextureImage(VulkanDevice& vkDev,
	const char* filename,
	//VkImage& textureImage, 
	//VkDeviceMemory& textureImageMemory, 
	uint32_t* outTexWidth,
	uint32_t* outTexHeight)
{
	return false;
}

bool VulkanTexture::CreateTextureSampler(VkDevice device,
	//VkSampler* sampler, 
	VkFilter minFilter,
	VkFilter maxFilter,
	VkSamplerAddressMode addressMode)
{
	return false;
}