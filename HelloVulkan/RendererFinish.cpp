#include "RendererFinish.h"
#include "VulkanUtility.h"

RendererFinish::RendererFinish(VulkanDevice& vkDev, VulkanImage* depthImage) : 
	RendererBase(vkDev, depthImage)
{
	CreateColorAndDepthRenderPass(
		vkDev,
		(depthImage != nullptr),
		&renderPass_,
		RenderPassCreateInfo
		{ 
			.clearColor_ = false, 
			.clearDepth_ = false, 
			.renderFirst_ = false,
			.renderLast_ = true,
		});

	CreateColorAndDepthFramebuffers(
		vkDev, 
		renderPass_, 
		depthImage_->imageView_, 
		swapchainFramebuffers_);
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