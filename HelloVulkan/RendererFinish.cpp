#include "RendererFinish.h"
#include "VulkanUtility.h"

// Present swapchain image 
RendererFinish::RendererFinish(VulkanDevice& vkDev, VulkanImage* depthImage) : 
	RendererBase(vkDev, depthImage)
{
	CreateOnScreenRenderPass(
		vkDev,
		&renderPass_, 
		// Present swapchain image 
		RenderPassBit::OnScreenColorPresent);

	CreateOnScreenFramebuffers(
		vkDev, 
		renderPass_, 
		depthImage_->imageView_);
}

void RendererFinish::FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage)
{
	const VkRect2D screenRect = {
		.offset = { 0, 0 },
		.extent = {.width = framebufferWidth_, .height = framebufferHeight_ }
	};

	const VkRenderPassBeginInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass_,
		.framebuffer = swapchainFramebuffers_[currentImage],
		.renderArea = screenRect
	};

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdEndRenderPass(commandBuffer);
}