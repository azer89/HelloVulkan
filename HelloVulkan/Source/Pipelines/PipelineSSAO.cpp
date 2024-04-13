#include "PipelineSSAO.h"

PipelineSSAO::PipelineSSAO(VulkanContext& ctx,
	ResourcesGBuffer* resourcesGBuffer,
	uint8_t renderBit) :
	PipelineBase(ctx,
	{
		.type_ = PipelineType::GraphicsOffScreen,
	}),
	resourcesGBuffer_(resourcesGBuffer)
{
	VulkanBuffer::CreateMultipleUniformBuffers(ctx, ssaoUboBuffers_, sizeof(SSAOUBO), AppConfig::FrameCount);
	CreateDescriptor(ctx);
	renderPass_.CreateOffScreenColorOnly(ctx, resourcesGBuffer_->ssao_.imageFormat_, renderBit | RenderPassBit::ColorClear);
	framebuffer_.CreateResizeable(ctx, renderPass_.GetHandle(), { &(resourcesGBuffer_->ssao_), }, IsOffscreen());
	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_);
	AddOverridingColorBlendAttachment(0xf, VK_FALSE);
	CreateGraphicsPipeline(
		ctx,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "Common/FullscreenTriangle.vert",
			AppConfig::ShaderFolder + "SSAO/SSAO.frag"
		},
		&pipeline_
	);
}

PipelineSSAO::~PipelineSSAO()
{
	for (auto& buff : ssaoUboBuffers_)
	{
		buff.Destroy();
	}
}

void PipelineSSAO::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "SSAO", tracy::Color::LightSeaGreen);
	const uint32_t frameIndex = ctx.GetFrameIndex();

	renderPass_.BeginRenderPass(ctx, commandBuffer, framebuffer_.GetFramebuffer());

	BindPipeline(ctx, commandBuffer);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout_,
		0u, // firstSet
		1u, // descriptorSetCount
		&(descriptorSets_[frameIndex]),
		0u, // dynamicOffsetCount
		nullptr); // pDynamicOffsets

	ctx.InsertDebugLabel(commandBuffer, "PipelineSSAO", 0xff9999ff);
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);

	VulkanImage::TransitionLayoutCommand(
		commandBuffer,
		resourcesGBuffer_->ssao_.image_,
		resourcesGBuffer_->ssao_.imageFormat_,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void PipelineSSAO::OnWindowResized(VulkanContext& ctx)
{
	PipelineBase::OnWindowResized(ctx);
	UpdateDescriptorSets(ctx);
}

void PipelineSSAO::CreateDescriptor(VulkanContext& ctx)
{
	descriptorInfo_.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 0
	descriptorInfo_.AddBuffer(&(resourcesGBuffer_->kernel_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 1
	descriptorInfo_.AddImage(nullptr); // 2
	descriptorInfo_.AddImage(nullptr); // 3
	descriptorInfo_.AddImage(&(resourcesGBuffer_->noise_)); // 4

	// Pool and layout
	descriptor_.CreatePoolAndLayout(ctx, descriptorInfo_, AppConfig::FrameCount, 1u);

	AllocateDescriptorSets(ctx);
	UpdateDescriptorSets(ctx);
}

void PipelineSSAO::AllocateDescriptorSets(VulkanContext& ctx)
{
	descriptorSets_.resize(AppConfig::FrameCount);
	for (uint32_t i = 0; i < AppConfig::FrameCount; ++i)
	{
		descriptorInfo_.UpdateBuffer(&(ssaoUboBuffers_[i]), 0);
		descriptor_.AllocateSet(ctx, &(descriptorSets_[i]));
	}
}

void PipelineSSAO::UpdateDescriptorSets(VulkanContext& ctx)
{
	constexpr uint32_t frameCount = AppConfig::FrameCount;
	descriptorInfo_.UpdateImage(&(resourcesGBuffer_->position_), 2); // 2
	descriptorInfo_.UpdateImage(&(resourcesGBuffer_->normal_), 3); // 3
	for (uint32_t i = 0; i < AppConfig::FrameCount; ++i)
	{
		descriptor_.UpdateSet(ctx, descriptorInfo_, &(descriptorSets_[i]));
	}
}