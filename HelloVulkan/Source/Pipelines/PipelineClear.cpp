#include "PipelineClear.h"
#include "VulkanUtility.h"

RendererClear::RendererClear(VulkanDevice& vkDev) :
	RendererBase(
		vkDev,
		false // Onscreen
		)
{
	renderPass_.CreateOnScreenColorOnlyRenderPass(vkDev, RenderPassBit::ColorClear);
	framebuffer_.Create(vkDev, renderPass_.GetHandle(), {}, isOffscreen_);
}

RendererClear::~RendererClear()
{
}

void RendererClear::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t swapchainImageIndex)
{
	renderPass_.BeginRenderPass(vkDev, commandBuffer, framebuffer_.GetFramebuffer(swapchainImageIndex));
	vkCmdEndRenderPass(commandBuffer);
}