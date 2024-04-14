#include "PipelineClear.h"

PipelineClear::PipelineClear(VulkanContext& ctx) :
	PipelineBase(
		ctx,
		{
			.type_ = PipelineType::GraphicsOnScreen
		})
{
	renderPass_.CreateOnScreenColorOnly(ctx, RenderPassBit::ColorClear);
	framebuffer_.CreateResizeable(ctx, renderPass_.GetHandle(), {}, IsOffscreen());
}

PipelineClear::~PipelineClear()
{
}

void PipelineClear::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "Clear", tracy::Color::DarkRed);
	const uint32_t swapchainImageIndex = ctx.GetCurrentSwapchainImageIndex();
	renderPass_.BeginRenderPass(ctx, commandBuffer, framebuffer_.GetFramebuffer(swapchainImageIndex));
	ctx.InsertDebugLabel(commandBuffer, "PipelineClear", 0xff99ffff);
	vkCmdEndRenderPass(commandBuffer);
}