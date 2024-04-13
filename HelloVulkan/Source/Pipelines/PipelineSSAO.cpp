#include "PipelineSSAO.h"

PipelineSSAO::PipelineSSAO(VulkanContext& ctx,
	ResourcesGBuffer* resourcesGBuffer,
	uint8_t renderBit) :
	PipelineBase(ctx,
	{
		.type_ = PipelineType::GraphicsOffScreen,
		.vertexBufferBind_ = false,
	}),
	resourcesGBuffer_(resourcesGBuffer)
{
	VulkanBuffer::CreateMultipleUniformBuffers(ctx, ssaoUboBuffers_, sizeof(SSAOUBO), AppConfig::FrameCount);
	renderPass_.CreateOnScreenColorOnlyRenderPass(ctx);
}

PipelineSSAO::~PipelineSSAO()
{
}

void PipelineSSAO::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
}

void PipelineSSAO::CreateDescriptor(VulkanContext& ctx)
{
	constexpr uint32_t frameCount = AppConfig::FrameCount;

	VulkanDescriptorInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	dsInfo.AddBuffer(&(resourcesGBuffer_->kernel_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	dsInfo.AddImage(&(resourcesGBuffer_->position_));
	dsInfo.AddImage(&(resourcesGBuffer_->normal_));
	dsInfo.AddImage(&(resourcesGBuffer_->noise_));

	// Pool and layout
	descriptor_.CreatePoolAndLayout(ctx, dsInfo, frameCount, 1u);

	// Sets
	descriptorSets_.resize(frameCount);
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(ssaoUboBuffers_[i]), 0);
		descriptor_.CreateSet(ctx, dsInfo, &(descriptorSets_[i]));
	}
}