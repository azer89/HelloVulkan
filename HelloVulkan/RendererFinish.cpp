#include "RendererFinish.h"
#include "VulkanUtility.h"

// Present swapchain image 
RendererFinish::RendererFinish(VulkanDevice& vkDev, VulkanImage* depthImage) : 
	RendererBase(vkDev, depthImage)
{
	renderPass_.CreateOnScreenRenderPass(
		vkDev, 
		// Present swapchain image 
		RenderPassBit::OnScreenColorPresent);

	CreateOnScreenFramebuffers(
		vkDev, 
		renderPass_, 
		depthImage_->imageView_);
}

void RendererFinish::RecordCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t swapchainImageIndex)
{
	renderPass_.BeginRenderPass(commandBuffer, swapchainFramebuffers_[swapchainImageIndex]);
	vkCmdEndRenderPass(commandBuffer);
}