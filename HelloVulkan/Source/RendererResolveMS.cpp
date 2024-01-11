#include "RendererResolveMS.h"

/*
Resolves a multisampled color image to a singlesampled color image
*/
RendererResolveMS::RendererResolveMS(
	VulkanDevice& vkDev,
	VulkanImage* multiSampledColorImage, // Input
	VulkanImage* singleSampledColorImage // Output
) :
	RendererBase(vkDev, true), // Offscreen
	multiSampledColorImage_(multiSampledColorImage),
	singleSampledColorImage_(singleSampledColorImage)
{
	renderPass_.CreateResolveMSRenderPass(
		vkDev,
		0u,
		multiSampledColorImage_->multisampleCount_);

	framebuffer_.Create(
		vkDev, 
		renderPass_.GetHandle(),
		{
			multiSampledColorImage_,
			singleSampledColorImage_
		},
		isOffscreen_);
}

RendererResolveMS::~RendererResolveMS()
{
}

void RendererResolveMS::FillCommandBuffer(
	VulkanDevice& vkDev,
	VkCommandBuffer commandBuffer,
	size_t currentImage)
{
	renderPass_.BeginRenderPass(vkDev, commandBuffer, framebuffer_.GetFramebuffer());
	vkCmdEndRenderPass(commandBuffer);
}