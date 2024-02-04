#include "PipelineFinish.h"
#include "VulkanUtility.h"

/*
	Present swapchain image 
*/
PipelineFinish::PipelineFinish(VulkanDevice& vkDev) :
	PipelineBase(vkDev,
		{
			.type_ = PipelineType::GraphicsOnScreen
		}
	)
{
	renderPass_.CreateOnScreenColorOnlyRenderPass(
		vkDev, 
		// Present swapchain image 
		RenderPassBit::ColorPresent);

	framebuffer_.Create(vkDev, renderPass_.GetHandle(), {}, IsOffscreen());
}

void PipelineFinish::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer)
{
	uint32_t swapchainImageIndex = vkDev.GetCurrentSwapchainImageIndex();
	renderPass_.BeginRenderPass(vkDev, commandBuffer, framebuffer_.GetFramebuffer(swapchainImageIndex));
	vkCmdEndRenderPass(commandBuffer);
}