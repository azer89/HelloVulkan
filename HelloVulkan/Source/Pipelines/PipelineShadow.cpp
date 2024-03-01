#include "PipelineShadow.h"

PipelineShadow::PipelineShadow(
	VulkanContext& ctx,
	Scene* scene,
	VulkanImage* shadowMap) :
	PipelineBase(ctx,
		{
			// Depth only and offscreen
			.type_ = PipelineType::GraphicsOffScreen,

			// If you use bindless, make sure this is false
			.vertexBufferBind_ = false,

			// Render using shadow map dimension
			.customViewportSize_ = true,
			.viewportWidth_ = static_cast<float>(shadowMap->width_),
			.viewportHeight_ = static_cast<float>(shadowMap->height_)
		}),
	scene_(scene),
	shadowMap_(shadowMap)
{
	CreateMultipleUniformBuffers(ctx, shadowMapUBOBuffers_, sizeof(ShadowMapUBO), AppConfig::FrameOverlapCount);

	renderPass_.CreateDepthOnlyRenderPass(ctx, 
		RenderPassBit::DepthClear | RenderPassBit::DepthShaderReadOnly);

	framebuffer_.CreateUnresizeable(
		ctx,
		renderPass_.GetHandle(),
		{
			// Use the shadow map as depth attachment
			shadowMap_->imageView_
		},
		shadowMap_->width_,
		shadowMap_->height_);

	CreateIndirectBuffers(ctx, scene_, indirectBuffers_);

	CreateDescriptor(ctx);

	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_);

	CreateGraphicsPipeline(
		ctx,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			// Just need a vertex shader
			AppConfig::ShaderFolder + "ShadowMapping//Depth.vert",
		},
		&pipeline_
	);
}

PipelineShadow::~PipelineShadow()
{
	for (auto& buffer : shadowMapUBOBuffers_)
	{
		buffer.Destroy();
	}

	for (auto& buffer : indirectBuffers_)
	{
		buffer.Destroy();
	}
}

void PipelineShadow::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	uint32_t frameIndex = ctx.GetFrameIndex();
	renderPass_.BeginRenderPass(
		ctx, 
		commandBuffer, 
		framebuffer_.GetFramebuffer(), 
		shadowMap_->width_,
		shadowMap_->height_);
	BindPipeline(ctx, commandBuffer);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout_,
		0u,
		1u,
		&descriptorSets_[frameIndex],
		0u,
		nullptr);

	vkCmdDrawIndirect(
		commandBuffer,
		indirectBuffers_[frameIndex].buffer_,
		0, // offset
		scene_->GetMeshCount(),
		sizeof(VkDrawIndirectCommand));

	vkCmdEndRenderPass(commandBuffer);
}


void PipelineShadow::OnWindowResized(VulkanContext& ctx)
{
}

void PipelineShadow::CreateDescriptor(VulkanContext& ctx)
{
	constexpr uint32_t frameCount = AppConfig::FrameOverlapCount;

	DescriptorBuildInfo buildInfo;
	buildInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 0
	buildInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 1
	buildInfo.AddBuffer(&(scene_->vertexBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 2
	buildInfo.AddBuffer(&(scene_->indexBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 3
	buildInfo.AddBuffer(&(scene_->meshDataBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 4

	// Create pool and layout
	descriptor_.CreatePoolAndLayout(ctx, buildInfo, frameCount, 1u);

	// Create sets
	descriptorSets_.resize(frameCount); // TODO use std::array
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		buildInfo.UpdateBuffer(&(shadowMapUBOBuffers_[i]), 0);
		buildInfo.UpdateBuffer(&(scene_->modelUBOBuffers_[i]), 1);
		descriptor_.CreateSet(ctx, buildInfo, &(descriptorSets_[i]));
	}
}