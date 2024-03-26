#include "PipelineShadow.h"
#include "ResourcesShadow.h"
#include "Scene.h"

#include <glm/gtc/matrix_transform.hpp>

PipelineShadow::PipelineShadow(
	VulkanContext& ctx,
	Scene* scene,
	ResourcesShadow* resourcesShadow) :
	PipelineBase(ctx,
	{
		// Depth only and offscreen
		.type_ = PipelineType::GraphicsOffScreen,
		.vertexBufferBind_ = false,
		.customViewportSize_ = true,
		// Render using shadow map dimension
		.viewportWidth_ = static_cast<float>(resourcesShadow->shadowMap_.width_),
		.viewportHeight_ = static_cast<float>(resourcesShadow->shadowMap_.height_)
	}),
	bda_(scene->GetBDA()),
	scene_(scene),
	resourcesShadow_(resourcesShadow)
{
	VulkanBuffer::CreateMultipleUniformBuffers(ctx, shadowMapUBOBuffers_, sizeof(ShadowMapUBO), AppConfig::FrameCount);
	renderPass_.CreateDepthOnlyRenderPass(ctx, 
		RenderPassBit::DepthClear | RenderPassBit::DepthShaderReadOnly);
	framebuffer_.CreateUnresizeable(
		ctx,
		renderPass_.GetHandle(),
		{
			// Use the shadow map as depth attachment
			resourcesShadow_->shadowMap_.imageView_
		},
		resourcesShadow_->shadowMap_.width_,
		resourcesShadow_->shadowMap_.height_);
	scene_->CreateIndirectBuffer(ctx, indirectBuffer_);
	CreateDescriptor(ctx);
	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_, sizeof(BDA), VK_SHADER_STAGE_VERTEX_BIT);
	CreateGraphicsPipeline(
		ctx,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "ShadowMapping/Depth.vert",
			AppConfig::ShaderFolder + "ShadowMapping/Depth.frag",
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
	indirectBuffer_.Destroy();
}

void PipelineShadow::UpdateShadow(VulkanContext& ctx, ResourcesShadow* resShadow, glm::vec4 lightPosition)
{
	//glm::mat4 lightProjection = 
	// glm::perspective(glm::radians(45.f), 1.0f, resShadow_.shadowNearPlane, resShadow_.shadowFarPlane);
	const glm::mat4 lightProjection = glm::ortho(
		-resourcesShadow_->orthoSize_,
		resourcesShadow_->orthoSize_,
		resourcesShadow_->orthoSize_,
		-resourcesShadow_->orthoSize_,
		resourcesShadow_->shadowNearPlane_,
		resourcesShadow_->shadowFarPlane_);
	const glm::mat4 lightView = glm::lookAt(glm::vec3(lightPosition), glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
	const glm::mat4 lightSpaceMatrix = lightProjection * lightView;
	resourcesShadow_->shadowUBO_.lightSpaceMatrix = lightSpaceMatrix;
	resourcesShadow_->shadowUBO_.lightPosition = lightPosition;

	const uint32_t frameIndex = ctx.GetFrameIndex();
	shadowMapUBOBuffers_[frameIndex].UploadBufferData(ctx, &(resourcesShadow_->shadowUBO_), sizeof(ShadowMapUBO));
}

void PipelineShadow::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "Render_Shadow_Map", tracy::Color::OrangeRed);

	const uint32_t frameIndex = ctx.GetFrameIndex();
	renderPass_.BeginRenderPass(
		ctx, 
		commandBuffer, 
		framebuffer_.GetFramebuffer(), 
		resourcesShadow_->shadowMap_.width_,
		resourcesShadow_->shadowMap_.height_);
	BindPipeline(ctx, commandBuffer);

	vkCmdPushConstants(
		commandBuffer,
		pipelineLayout_,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(BDA), &bda_);

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
		indirectBuffer_.buffer_,
		0, // offset
		scene_->GetInstanceCount(),
		sizeof(VkDrawIndirectCommand));

	vkCmdEndRenderPass(commandBuffer);
}


void PipelineShadow::OnWindowResized(VulkanContext& ctx)
{
}

void PipelineShadow::CreateDescriptor(VulkanContext& ctx)
{
	constexpr uint32_t frameCount = AppConfig::FrameCount;

	VulkanDescriptorInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 0
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 1

	// Pool and layout
	descriptor_.CreatePoolAndLayout(ctx, dsInfo, frameCount, 1u);

	// Sets
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(shadowMapUBOBuffers_[i]), 0);
		dsInfo.UpdateBuffer(&(scene_->modelSSBOBuffers_[i]), 1);
		descriptor_.CreateSet(ctx, dsInfo, &(descriptorSets_[i]));
	}
}