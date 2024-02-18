#include "PipelineShadow.h"

PipelineShadow::PipelineShadow(
	VulkanContext& ctx,
	std::vector<Model*> models,
	VulkanImage* shadowMap) :
	PipelineBase(ctx,
		{
			// Depth only and offscreen
			.type_ = PipelineType::GraphicsOffScreen,
			.vertexBufferBind_ = true,

			// Render using shadow map dimension
			.customViewportSize_ = true,
			.viewportWidth_ = static_cast<float>(shadowMap->width_),
			.viewportHeight_ = static_cast<float>(shadowMap->height_)
		}),
	models_(models),
	shadowMap_(shadowMap)
{
	CreateMultipleUniformBuffers(ctx, shadowMapUBOBuffers_, sizeof(ShadowMapUBO), AppConfig::FrameOverlapCount);

	renderPass_.CreateDepthOnlyRenderPass(ctx, RenderPassBit::DepthClear);

	framebuffer_.Create(
		ctx,
		renderPass_.GetHandle(),
		{
			// Use the shadow map as depth attachment
			shadowMap_
		},
		IsOffscreen());

	CreateDescriptor(ctx);

	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_);

	CreateGraphicsPipeline(
		ctx,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "ShadowMapping//Depth.vert",
			AppConfig::ShaderFolder + "ShadowMapping//Depth.frag"
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
}

void PipelineShadow::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	uint32_t frameIndex = ctx.GetFrameIndex();
	renderPass_.BeginRenderPass(ctx, commandBuffer, framebuffer_.GetFramebuffer());
	BindPipeline(ctx, commandBuffer);
	
	for (size_t i = 0; i < models_.size(); ++i)
	{
		size_t index = i * AppConfig::FrameOverlapCount;
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout_,
			0,
			1,
			&descriptorSets_[index + frameIndex],
			0,
			nullptr);
		for (Mesh& mesh : models_[i]->meshes_)
		{
			// Bind vertex buffer
			VkBuffer buffers[] = { mesh.vertexBuffer_.buffer_ };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

			// Bind index buffer
			vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer_.buffer_, 0, VK_INDEX_TYPE_UINT32);

			// Draw
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.indexBufferSize_ / (sizeof(unsigned int))), 1, 0, 0, 0);
		}
	}

	vkCmdEndRenderPass(commandBuffer);
}

void PipelineShadow::CreateDescriptor(VulkanContext& ctx)
{
	descriptor_.CreatePool(
		ctx,
		{
			.uboCount_ = 2u,
			.frameCount_ = AppConfig::FrameOverlapCount,
			.setCountPerFrame_ = static_cast<uint32_t>(models_.size()),
		});

	// Layout
	descriptor_.CreateLayout(ctx,
		{
			{
				.descriptorType_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT,
				.bindingCount_ = 2
			}
		});

	// Set
	descriptorSets_.resize(models_.size() * AppConfig::FrameOverlapCount);
	for (size_t i = 0; i < models_.size(); ++i)
	{
		size_t index = i * AppConfig::FrameOverlapCount;
		for (size_t j = 0; j < AppConfig::FrameOverlapCount; ++j)
		{
			VkDescriptorBufferInfo bufferInfo1 = { shadowMapUBOBuffers_[j].buffer_, 0, sizeof(ShadowMapUBO)};
			VkDescriptorBufferInfo bufferInfo2 = { models_[i]->modelBuffers_[i].buffer_, 0, sizeof(ModelUBO)};

			descriptor_.CreateSet(
				ctx,
				{
					{.bufferInfoPtr_ = &bufferInfo1, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
					{.bufferInfoPtr_ = &bufferInfo2, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
				},
				&(descriptorSets_[index + j]));
		}
	}
}