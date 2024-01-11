#include "RendererLight.h"

RendererLight::RendererLight(
	VulkanDevice& vkDev,
	std::vector<Light>& lights) :
	RendererBase(vkDev, true),
	lights_(lights)
{

}

RendererLight::~RendererLight()
{

}

void RendererLight::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage)
{

}