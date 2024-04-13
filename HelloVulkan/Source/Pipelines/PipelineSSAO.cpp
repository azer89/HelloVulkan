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
	CreateDescriptor(ctx);
	renderPass_.CreateOffScreenColorOnly(ctx, resourcesGBuffer_->ssao_.imageFormat_, renderBit | RenderPassBit::ColorClear);
	framebuffer_.CreateResizeable(ctx, renderPass_.GetHandle(), { &(resourcesGBuffer_->ssao_), }, IsOffscreen());
	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_);
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