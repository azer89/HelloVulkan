#include "RendererIrradiance.h"

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