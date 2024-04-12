#include "PipelineGBuffer.h"

PipelineGBuffer::PipelineGBuffer(VulkanContext& ctx,
	Scene* scene,
	ResourcesGBuffer* resourcesGBuffer,
	uint8_t renderBit) :
	PipelineBase(ctx,
	{
		.type_ = PipelineType::GraphicsOffScreen,
		.vertexBufferBind_ = false,
	})
{
}

PipelineGBuffer::~PipelineGBuffer()
{
}

void PipelineGBuffer::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
}

void PipelineGBuffer::CreateBDABuffer(VulkanContext& ctx)
{
}

void PipelineGBuffer::CreateDescriptor(VulkanContext& ctx)
{
}