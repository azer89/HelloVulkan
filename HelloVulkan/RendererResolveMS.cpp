#include "RendererResolveMS.h"

// MSAA
RendererResolveMS::RendererResolveMS(
	VulkanDevice& vkDev,
	VulkanImage* multiSampledColorImage, // Input
	VulkanImage* singleSampledColorImage // Output
) :
	RendererBase(vkDev, nullptr),
	multiSampledColorImage_(multiSampledColorImage),
	singleSampledColorImage_(singleSampledColorImage)
{
	renderPass_.CreateResolveMSRenderPass(
		vkDev,
		0u,
		multiSampledColorImage_->multisampleCount_);
	CreateSingleFramebuffer(
		vkDev,
		renderPass_,
		{
			multiSampledColorImage_->imageView_,
			singleSampledColorImage_->imageView_ 
		},
		offscreenFramebuffer_);
}

RendererResolveMS::~RendererResolveMS()
{
}

void RendererResolveMS::OnWindowResized(VulkanDevice& vkDev)
{
	vkDestroyFramebuffer(vkDev.GetDevice(), offscreenFramebuffer_, nullptr);
	CreateSingleFramebuffer(
		vkDev,
		renderPass_,
		{
			multiSampledColorImage_->imageView_,
			singleSampledColorImage_->imageView_
		},
		offscreenFramebuffer_);
}

void RendererResolveMS::FillCommandBuffer(
	VulkanDevice& vkDev,
	VkCommandBuffer commandBuffer,
	size_t currentImage)
{
	renderPass_.BeginRenderPass(vkDev, commandBuffer, offscreenFramebuffer_);
	vkCmdEndRenderPass(commandBuffer);
}