#include "RendererResolveMultisampling.h"

RendererResolveMultisampling::RendererResolveMultisampling(
	VulkanDevice& vkDev,
	VulkanImage* multiSampledColorImage, // Input
	VulkanImage* singleSampledColorImage // Output
) :
	RendererBase(vkDev, nullptr)
{
	renderPass_.CreateMultisampleResolveRenderPass(
		vkDev);

	CreateResolveMultisampingFramebuffer(
		vkDev,
		renderPass_,
		multiSampledColorImage->imageView_,
		singleSampledColorImage->imageView_,
		offscreenFramebuffer_);
}

void RendererResolveMultisampling::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage)
{
	renderPass_.BeginRenderPass(commandBuffer, offscreenFramebuffer_);
	vkCmdEndRenderPass(commandBuffer);
}