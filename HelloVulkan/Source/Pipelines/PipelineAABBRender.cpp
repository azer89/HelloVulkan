#include "PipelineAABBRender.h"
#include "ResourcesShared.h"
#include "Scene.h"

PipelineAABBRender::PipelineAABBRender(
	VulkanContext& ctx,
	Scene* scene,
	ResourcesShared* resShared,
	uint8_t renderBit) :
	PipelineBase(ctx,
	{
		.type_ = PipelineType::GraphicsOffScreen,
		.msaaSamples_ = resShared->multiSampledColorImage_.multisampleCount_,
		.depthTest_ = true,
		.depthWrite_ = false
	}
	)
{

}

PipelineAABBRender::~PipelineAABBRender()
{
}

void PipelineAABBRender::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
}

void PipelineAABBRender::CreateDescriptor(VulkanContext& ctx)
{
}