#include "PipelineClear.h"
#include "VulkanUtility.h"

PipelineClear::PipelineClear(VulkanContext& ctx) :
	PipelineBase(
		ctx,
		{
			.type_ = PipelineType::GraphicsOnScreen
		})
{
	renderPass_.CreateOnScreenColorOnlyRenderPass(ctx, RenderPassBit::ColorClear);
	framebuffer_.Create(ctx, renderPass_.GetHandle(), {}, IsOffscreen());
}

PipelineClear::~PipelineClear()
{
}

void PipelineClear::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	uint32_t swapchainImageIndex = ctx.GetCurrentSwapchainImageIndex();
	renderPass_.BeginRenderPass(ctx, commandBuffer, framebuffer_.GetFramebuffer(swapchainImageIndex));
	vkCmdEndRenderPass(commandBuffer);
}