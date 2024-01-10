#include "RendererClear.h"
#include "VulkanUtility.h"

RendererClear::RendererClear(VulkanDevice& vkDev) :
	RendererBase(
		vkDev,
		false // isOffscreen
		)
{
	renderPass_.CreateOnScreenColorOnlyRenderPass(vkDev, RenderPassBit::ColorClear);

	//CreateSwapchainFramebuffers(
	//	vkDev,
	//	renderPass_);
	framebuffer_.Create(vkDev, renderPass_.GetHandle(), {}, isOffscreen_);
}

RendererClear::~RendererClear()
{
}

void RendererClear::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t swapchainImageIndex)
{
	//renderPass_.BeginRenderPass(vkDev, commandBuffer, swapchainFramebuffers_[swapchainImageIndex]);
	renderPass_.BeginRenderPass(vkDev, commandBuffer, framebuffer_.GetFramebuffer(swapchainImageIndex));
	vkCmdEndRenderPass(commandBuffer);
}