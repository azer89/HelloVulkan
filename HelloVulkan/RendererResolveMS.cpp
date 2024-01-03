#include "RendererResolveMS.h"

// MSAA
RendererResolveMS::RendererResolveMS(
	VulkanDevice& vkDev,
	VulkanImage* multiSampledColorImage, // Input
	VulkanImage* singleSampledColorImage // Output
) :
	RendererBase(vkDev, nullptr)
{
	renderPass_.CreateResolveMSRenderPass(
		vkDev,
		0u,
		multiSampledColorImage->multisampleCount_);

	CreateResolveMSFramebuffer(
		vkDev,
		renderPass_,
		multiSampledColorImage->imageView_,
		singleSampledColorImage->imageView_,
		offscreenFramebuffer_);
}

RendererResolveMS::~RendererResolveMS()
{
}

void RendererResolveMS::FillCommandBuffer(
	VulkanDevice& vkDev,
	VkCommandBuffer commandBuffer,
	size_t currentImage)
{
	renderPass_.BeginRenderPass(commandBuffer, offscreenFramebuffer_);
	vkCmdEndRenderPass(commandBuffer);
}