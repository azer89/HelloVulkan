#include "PipelineSSAO.h"

PipelineSSAO::PipelineSSAO(VulkanContext& ctx,
	ResourcesGBuffer* resourcesGBuffer,
	uint8_t renderBit) :
	PipelineBase(ctx,
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.vertexBufferBind_ = false,
		})
{
}

PipelineSSAO::~PipelineSSAO()
{
}

void PipelineSSAO::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
}

void PipelineSSAO::CreateDescriptor(VulkanContext& ctx)
{
}