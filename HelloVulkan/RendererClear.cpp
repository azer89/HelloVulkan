#include "RendererClear.h"
#include "VulkanUtility.h"

RendererClear::RendererClear(VulkanDevice& vkDev, VulkanImage* depthImage) : 
	RendererBase(vkDev, depthImage), 
	shouldClearDepth_(depthImage != nullptr)
{
	CreateOnscreenRenderPass(
		vkDev,
		shouldClearDepth_,
		&renderPass_,
		RenderPassType::First);

	CreateOnscreenFramebuffers(
		vkDev, 
		renderPass_, 
		depthImage_->imageView_, 
		swapchainFramebuffers_);
}

void RendererClear::FillCommandBuffer(VkCommandBuffer commandBuffer, size_t swapFramebuffer)
{
	const VkClearValue clearValues[2] =
	{
		VkClearValue {.color = { 1.0f, 1.0f, 1.0f, 1.0f } },
		VkClearValue {.depthStencil = { 1.0f, 0 } }
	};

	const VkRect2D screenRect = {
		.offset = { 0, 0 },
		.extent = {.width = framebufferWidth_, .height = framebufferHeight_ }
	};

	const VkRenderPassBeginInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = renderPass_,
		.framebuffer = swapchainFramebuffers_[swapFramebuffer],
		.renderArea = screenRect,
		.clearValueCount = static_cast<uint32_t>(shouldClearDepth_ ? 2 : 1),
		.pClearValues = &clearValues[0]
	};

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdEndRenderPass(commandBuffer);
}