#include "PipelineResolveMS.h"
#include "ResourcesShared.h"

/*
Resolves a multisampled color image to a singlesampled color image
*/
PipelineResolveMS::PipelineResolveMS(
	VulkanContext& ctx,
	ResourcesShared* resourcesShared) :
	PipelineBase(ctx, 
		{ .type_ = PipelineType::GraphicsOffScreen }
	)
{
	renderPass_.CreateResolveMSRenderPass(
		ctx,
		0u,
		resourcesShared->multiSampledColorImage_.multisampleCount_);

	framebuffer_.CreateResizeable(
		ctx, 
		renderPass_.GetHandle(),
		{
			&(resourcesShared->multiSampledColorImage_),
			&(resourcesShared->singleSampledColorImage_)
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
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "Multisample_Resolve", tracy::Color::GreenYellow);
	renderPass_.BeginRenderPass(ctx, commandBuffer, framebuffer_.GetFramebuffer());
	ctx.InsertDebugLabel(commandBuffer, "PipelineResolveMS", 0xffff99ff);
	vkCmdEndRenderPass(commandBuffer);
}