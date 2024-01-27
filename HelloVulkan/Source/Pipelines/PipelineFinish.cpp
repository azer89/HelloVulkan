#include "PipelineFinish.h"
#include "VulkanUtility.h"

/*
	Present swapchain image 
*/
PipelineFinish::PipelineFinish(VulkanDevice& vkDev) : 
	PipelineBase(vkDev, 
		false) // Onscreen
{
	renderPass_.CreateOnScreenColorOnlyRenderPass(
		vkDev, 
		// Present swapchain image 
		RenderPassBit::ColorPresent);

	framebuffer_.Create(vkDev, renderPass_.GetHandle(), {}, isOffscreen_);
}

void PipelineFinish::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t swapchainImageIndex)
{
	renderPass_.BeginRenderPass(vkDev, commandBuffer, framebuffer_.GetFramebuffer(swapchainImageIndex));
	vkCmdEndRenderPass(commandBuffer);
}