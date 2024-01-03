#include "RendererClear.h"
#include "VulkanUtility.h"

RendererClear::RendererClear(VulkanDevice& vkDev) :
	RendererBase(vkDev, nullptr)
{
	renderPass_.CreateOnScreenColorOnlyRenderPass(vkDev, RenderPassBit::ColorClear);

	CreateOnScreenFramebuffers(
		vkDev,
		renderPass_);
}

RendererClear::~RendererClear()
{
}

void RendererClear::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t swapchainImageIndex)
{
	renderPass_.BeginRenderPass(commandBuffer, swapchainFramebuffers_[swapchainImageIndex]);
	vkCmdEndRenderPass(commandBuffer);
}