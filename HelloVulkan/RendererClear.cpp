#include "RendererClear.h"
#include "VulkanUtility.h"

// Clear color and depth
RendererClear::RendererClear(VulkanDevice& vkDev, VulkanImage* depthImage) :
	RendererBase(vkDev, depthImage),
	shouldClearDepth_(depthImage != nullptr)
{
	renderPass_.CreateOnScreenRenderPass(
		vkDev,
		// Clear color and depth
		RenderPassBit::OnScreenColorClear | RenderPassBit::OnScreenDepthClear);

	CreateOnScreenFramebuffers(
		vkDev, 
		renderPass_, 
		depthImage_->imageView_);
}

void RendererClear::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t swapchainImageIndex)
{
	/*const VkClearValue clearValues[2] =
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
	};*/
	renderPass_.BeginRenderPass(commandBuffer, swapchainFramebuffers_[swapchainImageIndex]);
	//vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdEndRenderPass(commandBuffer);
}