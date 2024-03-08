#include "PipelineShadow.h"
#include "ResourcesShadow.h"
#include "Scene.h"

#include <glm/gtc/matrix_transform.hpp>

PipelineShadow::PipelineShadow(
	VulkanContext& ctx,
	Scene* scene,
	ResourcesShadow* resShadow) :
	PipelineBase(ctx,
		{
			// Depth only and offscreen
			.type_ = PipelineType::GraphicsOffScreen,

			// If you use bindless texture, make sure this is false
			.vertexBufferBind_ = false,

			// Render using shadow map dimension
			.customViewportSize_ = true,
			.viewportWidth_ = static_cast<float>(resShadow->shadowMap_.width_),
			.viewportHeight_ = static_cast<float>(resShadow->shadowMap_.height_)
		}),
	scene_(scene),
	resShadow_(resShadow)
{
	CreateMultipleUniformBuffers(ctx, shadowMapUBOBuffers_, sizeof(ShadowMapUBO), AppConfig::FrameOverlapCount);

	renderPass_.CreateDepthOnlyRenderPass(ctx, 
		RenderPassBit::DepthClear | RenderPassBit::DepthShaderReadOnly);

	framebuffer_.CreateUnresizeable(
		ctx,
		renderPass_.GetHandle(),
		{
			// Use the shadow map as depth attachment
			resShadow_->shadowMap_.imageView_
		},
		resShadow_->shadowMap_.width_,
		resShadow_->shadowMap_.height_);

	CreateIndirectBuffers(ctx, scene_, indirectBuffers_);

	CreateDescriptor(ctx);

	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_);

	CreateGraphicsPipeline(
		ctx,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "ShadowMapping//Depth.vert",
			AppConfig::ShaderFolder + "ShadowMapping//Depth.frag",
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

void PipelineShadow::UpdateShadow(VulkanContext& ctx, ResourcesShadow* resShadow, glm::vec4 lightPosition)
{
	//glm::mat4 lightProjection = 
	// glm::perspective(glm::radians(45.f), 1.0f, resShadow_.shadowNearPlane, resShadow_.shadowFarPlane);
	glm::mat4 lightProjection = glm::ortho(
		-resShadow_->orthoSize_,
		resShadow_->orthoSize_,
		-resShadow_->orthoSize_,
		resShadow_->orthoSize_,
		resShadow_->shadowNearPlane_,
		resShadow_->shadowFarPlane_);
	glm::mat4 lightView = glm::lookAt(glm::vec3(lightPosition), glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
	glm::mat4 lightSpaceMatrix = lightProjection * lightView;
	resShadow_->shadowUBO_.lightSpaceMatrix = lightSpaceMatrix;
	resShadow_->shadowUBO_.lightPosition = lightPosition;

	const uint32_t frameIndex = ctx.GetFrameIndex();
	shadowMapUBOBuffers_[frameIndex].UploadBufferData(ctx, &(resShadow_->shadowUBO_), sizeof(ShadowMapUBO));
}

void PipelineShadow::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "Render_Shadow_Map", tracy::Color::OrangeRed);

	uint32_t frameIndex = ctx.GetFrameIndex();
	renderPass_.BeginRenderPass(
		ctx, 
		commandBuffer, 
		framebuffer_.GetFramebuffer(), 
		resShadow_->shadowMap_.width_,
		resShadow_->shadowMap_.height_);
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

	VulkanDescriptorInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 0
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 1
	dsInfo.AddBuffer(&(scene_->vertexBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 2
	dsInfo.AddBuffer(&(scene_->indexBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 3
	dsInfo.AddBuffer(&(scene_->meshDataBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 4

	// Pool and layout
	descriptor_.CreatePoolAndLayout(ctx, dsInfo, frameCount, 1u);

	// Sets
	descriptorSets_.resize(frameCount); // TODO use std::array
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(shadowMapUBOBuffers_[i]), 0);
		dsInfo.UpdateBuffer(&(scene_->modelSSBOBuffers_[i]), 1);
		descriptor_.CreateSet(ctx, dsInfo, &(descriptorSets_[i]));
	}
}