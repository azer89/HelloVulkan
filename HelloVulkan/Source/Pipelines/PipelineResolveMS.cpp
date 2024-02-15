#include "PipelineResolveMS.h"

/*
Resolves a multisampled color image to a singlesampled color image
*/
PipelineResolveMS::PipelineResolveMS(
	VulkanContext& vkDev,
	VulkanImage* multiSampledColorImage, // Input
	VulkanImage* singleSampledColorImage // Output
) :
	PipelineBase(vkDev, 
		{ .type_ = PipelineType::GraphicsOffScreen }
	)
{
	renderPass_.CreateResolveMSRenderPass(
		vkDev,
		0u,
		multiSampledColorImage->multisampleCount_);

	framebuffer_.Create(
		vkDev, 
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
	VulkanContext& vkDev,
	VkCommandBuffer commandBuffer)
{
	renderPass_.BeginRenderPass(vkDev, commandBuffer, framebuffer_.GetFramebuffer());
	vkCmdEndRenderPass(commandBuffer);
}