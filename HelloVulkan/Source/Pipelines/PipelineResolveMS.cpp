#include "PipelineResolveMS.h"

/*
Resolves a multisampled color image to a singlesampled color image
*/
PipelineResolveMS::PipelineResolveMS(
	VulkanContext& ctx,
	ResourcesShared* resShared) :
	PipelineBase(ctx, 
		{ .type_ = PipelineType::GraphicsOffScreen }
	)
{
	renderPass_.CreateResolveMSRenderPass(
		ctx,
		0u,
		resShared->multiSampledColorImage_.multisampleCount_);

	framebuffer_.CreateResizeable(
		ctx, 
		renderPass_.GetHandle(),
		{
			//multiSampledColorImage,
			//singleSampledColorImage
			&(resShared->multiSampledColorImage_),
			&(resShared->singleSampledColorImage_)
		},
		IsOffscreen());
}

PipelineResolveMS::~PipelineResolveMS()
{
}

void PipelineResolveMS::FillCommandBuffer(
	VulkanContext& ctx,
	VkCommandBuffer commandBuffer)
{
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "Resolve_Multisample", tracy::Color::GreenYellow);
	renderPass_.BeginRenderPass(ctx, commandBuffer, framebuffer_.GetFramebuffer());
	vkCmdEndRenderPass(commandBuffer);
}