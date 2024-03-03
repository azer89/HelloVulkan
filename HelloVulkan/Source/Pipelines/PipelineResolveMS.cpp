#include "PipelineResolveMS.h"

/*
Resolves a multisampled color image to a singlesampled color image
*/
PipelineResolveMS::PipelineResolveMS(
	VulkanContext& ctx,
	// Resolve multiSampledColorImage to singleSampledColorImage
	VulkanImage* multiSampledColorImage, // Input
	VulkanImage* singleSampledColorImage // Output
) :
	PipelineBase(ctx, 
		{ .type_ = PipelineType::GraphicsOffScreen }
	)
{
	renderPass_.CreateResolveMSRenderPass(
		ctx,
		0u,
		multiSampledColorImage->multisampleCount_);

	framebuffer_.CreateResizeable(
		ctx, 
		renderPass_.GetHandle(),
		{
			multiSampledColorImage,
			singleSampledColorImage
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
	renderPass_.BeginRenderPass(ctx, commandBuffer, framebuffer_.GetFramebuffer());
	vkCmdEndRenderPass(commandBuffer);
}