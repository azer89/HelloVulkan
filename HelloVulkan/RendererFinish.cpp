#include "RendererFinish.h"
#include "VulkanUtility.h"

#include <iostream>

RendererFinish::RendererFinish(VulkanDevice& vkDev, VulkanImage* depthImage) : 
	RendererBase(vkDev, depthImage)
{
	if (!CreateColorAndDepthRenderPass(
		vkDev, 
		(depthImage != nullptr),
		&renderPass_, 
		RenderPassCreateInfo{ .clearColor_ = false, .clearDepth_ = false, .flags_ = eRenderPassBit_Last }))
	{
		std::cerr << "VulkanFinish: failed to create render pass\n";
	}

	CreateColorAndDepthFramebuffers(vkDev, renderPass_, depthImage_->imageView_, swapchainFramebuffers_);
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