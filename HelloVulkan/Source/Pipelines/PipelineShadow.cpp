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

	// Pool
	descriptor_.CreatePool(
		ctx,
		{
			.uboCount_ = 1u,
			.ssboCount_ = 4u,
			.frameCount_ = frameCount,
			.setCountPerFrame_ = 1u
		});

	// Layout
	descriptor_.CreateLayout(
		ctx,
		{
			{
				.type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT,
				.bindingCount_ = 1
			},
			{
				.type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT,
				.bindingCount_ = 4
			}
		});

	// Sets
	/*2*/ VkDescriptorBufferInfo vertexBufferInfo = { scene_->vertexBuffer_.buffer_, 0, scene_->vertexBuffer_.size_ };
	/*3*/ VkDescriptorBufferInfo indexBufferInfo = { scene_->indexBuffer_.buffer_, 0, scene_->indexBuffer_.size_ };
	/*4*/ VkDescriptorBufferInfo meshBufferInfo = { scene_->meshDataBuffer_.buffer_, 0, scene_->meshDataBuffer_.size_ };

	descriptorSets_.resize(frameCount);
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		/*0*/ VkDescriptorBufferInfo shadowBufferInfo = { shadowMapUBOBuffers_[i].buffer_, 0, sizeof(ShadowMapUBO) };
		/*1*/ VkDescriptorBufferInfo modelBufferInfo = { scene_->modelUBOBuffers_[i].buffer_, 0, scene_->modelUBOBuffers_[i].size_ };
	
		std::vector<DescriptorSetWrite> writes;
		/*0*/ writes.push_back({ .bufferInfoPtr_ = &shadowBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });
		/*1*/ writes.push_back({ .bufferInfoPtr_ = &modelBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*2*/ writes.push_back({ .bufferInfoPtr_ = &vertexBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*3*/ writes.push_back({ .bufferInfoPtr_ = &indexBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*4*/ writes.push_back({ .bufferInfoPtr_ = &meshBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });

		descriptor_.CreateSet(ctx, writes, &(descriptorSets_[i]));
	}
}