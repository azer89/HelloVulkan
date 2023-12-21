#include "RendererIrradiance.h"

// Irradiance map has only one mipmap level
const uint32_t mipmapCount = 1u;
const uint32_t cubemapSideLength = 1024;
const VkFormat cubeMapFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
const uint32_t layerCount = 6;

RendererIrradiance::RendererIrradiance(
	VulkanDevice& vkDev) :
	RendererBase(vkDev, nullptr)
{
}

RendererIrradiance::~RendererIrradiance()
{
	vkDestroyFramebuffer(device_, frameBuffer_, nullptr);
}

void RendererIrradiance::FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage)
{
}

void RendererIrradiance::InitializeIrradianceTexture(VulkanDevice& vkDev, VulkanTexture* irradianceTexture)
{
	irradianceTexture->image_.CreateImage(
		vkDev.GetDevice(),
		vkDev.GetPhysicalDevice(),
		cubemapSideLength,
		cubemapSideLength,
		mipmapCount,
		layerCount,
		cubeMapFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
	);
}

void RendererIrradiance::OfflineRender(VulkanDevice& vkDev, VulkanTexture* cubemapTexture, VulkanTexture* irradianceTexture)
{
	InitializeIrradianceTexture(vkDev, irradianceTexture);

}