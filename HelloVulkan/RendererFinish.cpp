#include "RendererFinish.h"
#include "VulkanUtility.h"

// Present swapchain image 
RendererFinish::RendererFinish(VulkanDevice& vkDev) : 
	RendererBase(vkDev, nullptr)
{
	renderPass_.CreateOnScreenColorOnlyRenderPass(
		vkDev, 
		// Present swapchain image 
		RenderPassBit::OnScreenColorPresent);

	CreateOnScreenFramebuffers(
		vkDev, 
		renderPass_);
}

void RendererFinish::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t swapchainImageIndex)
{
	renderPass_.BeginRenderPass(commandBuffer, swapchainFramebuffers_[swapchainImageIndex]);
	vkCmdEndRenderPass(commandBuffer);
}