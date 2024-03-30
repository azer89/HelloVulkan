#include "PipelineFinish.h"

/*
	Present swapchain image 
*/
PipelineFinish::PipelineFinish(VulkanContext& ctx) :
	PipelineBase(ctx,
		{
			.type_ = PipelineType::GraphicsOnScreen
		}
	)
{
	renderPass_.CreateOnScreenColorOnlyRenderPass(
		ctx, 
		// Present swapchain image 
		RenderPassBit::ColorPresent);
	framebuffer_.CreateResizeable(ctx, renderPass_.GetHandle(), {}, IsOffscreen());
}

void PipelineFinish::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "Finish", tracy::Color::MediumAquamarine);
	const uint32_t swapchainImageIndex = ctx.GetCurrentSwapchainImageIndex();
	renderPass_.BeginRenderPass(ctx, commandBuffer, framebuffer_.GetFramebuffer(swapchainImageIndex));
	ctx.InsertDebugLabel(commandBuffer, "PipelineFinish", 0xff99ffff);
	vkCmdEndRenderPass(commandBuffer);
}