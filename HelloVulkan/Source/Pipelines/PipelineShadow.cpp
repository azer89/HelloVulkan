#include "PipelineShadow.h"
#include "ResourcesLight.h"
#include "PushConstants.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

PipelineShadow::PipelineShadow(
	VulkanContext& ctx,
	Scene* scene,
	ResourcesShadow* resShadow) :
	PipelineBase(ctx,
		{
			// Depth only and offscreen
			.type_ = PipelineType::GraphicsOffScreen,

			// If you use bindless, make sure this is false
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

	for (uint32_t i = 0; i < ShadowConfig::CascadeCount; ++i)
	{
		cascadeFramebuffers_[i].CreateUnresizeable(
			ctx,
			renderPass_.GetHandle(),
			{
				// Use the shadow map as depth attachment
				resShadow_->views_[i]
			},
			resShadow_->shadowMap_.width_,
			resShadow_->shadowMap_.height_);
	}

	CreateIndirectBuffers(ctx, scene_, indirectBuffers_);

	CreateDescriptor(ctx);

	// Push constants
	const std::vector<VkPushConstantRange> ranges =
	{{
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.offset = 0u,
		.size = sizeof(PushConstCascadeShadow),
	}};

	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_, ranges);

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

	for (auto& framebuffer : cascadeFramebuffers_)
	{
		framebuffer.Destroy();
	}
}

void PipelineShadow::CalculateCascade(
	VulkanContext& ctx,
	const Camera* camera,
	const LightData* light,
	ShadowMapUBO* ubo)
{
	// TODO Set this as a paremeter
	constexpr float cascadeSplitLambda = 0.95f;
	constexpr uint32_t cascadeCount = ShadowConfig::CascadeCount;
	
	const glm::mat4 invCam = glm::inverse(camera->GetProjectionMatrix() * camera->GetViewMatrix());
	const glm::vec3 normLightPos = normalize(glm::vec3(light->position_.x, light->position_.y, light->position_.z));
	const float lightDist = glm::length(glm::vec3(light->position_));
	const float clipNear = CameraConfig::Near;
	const float clipFar = CameraConfig::Far;
	const float clipRange = clipFar - clipNear;
	const float clipRatio = clipFar / clipNear;

	float cascadeSplits[cascadeCount] = { 0 };

	// Calculate split depths based on view camera frustum
	// Based on method presented in developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
	for (uint32_t a = 0; a < ShadowConfig::CascadeCount; a++)
	{
		float p = (a + 1) / static_cast<float>(ShadowConfig::CascadeCount);
		float cLog = clipNear * std::pow(clipRatio, p);
		float cUniform = clipNear + clipRange * p;
		float d = cascadeSplitLambda * (cLog - cUniform) + cUniform;
		cascadeSplits[a] = (d - clipNear) / clipRange;
	}

	// Calculate orthographic projection matrix for each cascade
	float lastSplitDist = 0.0;
	for (uint32_t a = 0; a < ShadowConfig::CascadeCount; a++)
	{
		float splitDist = cascadeSplits[a];

		glm::vec3 frustumCorners[8] = {
			glm::vec3(-1.0f,  1.0f, 0.0f),
			glm::vec3(1.0f,  1.0f, 0.0f),
			glm::vec3(1.0f, -1.0f, 0.0f),
			glm::vec3(-1.0f, -1.0f, 0.0f),
			glm::vec3(-1.0f,  1.0f,  1.0f),
			glm::vec3(1.0f,  1.0f,  1.0f),
			glm::vec3(1.0f, -1.0f,  1.0f),
			glm::vec3(-1.0f, -1.0f,  1.0f),
		};

		// Project frustum corners into world space
		for (uint32_t i = 0; i < 8; i++)
		{
			glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
			frustumCorners[i] = invCorner / invCorner.w;
		}

		for (uint32_t i = 0; i < 4; i++)
		{
			glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
			frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
			frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
		}

		// Get frustum center
		glm::vec3 frustumCenter = glm::vec3(0.0f);
		for (uint32_t i = 0; i < 8; i++)
		{
			frustumCenter += frustumCorners[i];
		}
		frustumCenter /= 8.0f;

		float radius = 0.0f;
		for (uint32_t i = 0; i < 8; i++)
		{
			float distance = glm::length(frustumCorners[i] - frustumCenter);
			radius = glm::max(radius, distance);
		}
		radius = std::ceil(radius * 16.0f) / 16.0f;

		glm::vec3 eye = frustumCenter + glm::vec3(light->position_);
		glm::vec3 target = frustumCenter;
		glm::mat4 lightViewMatrix = glm::lookAt(eye, target, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 lightOrthoMatrix = glm::ortho(
			-radius,
			radius,
			-radius,
			radius,
			0.f,
			lightDist + radius);

		// Store split distance and matrix in cascade
		ubo->lightSpaceMatrices[a] = lightOrthoMatrix * lightViewMatrix;
		ubo->splitValues[a] = (clipNear + splitDist * clipRange) * -1.0f;
		
		lastSplitDist = cascadeSplits[a];
	}

	ubo->lightPosition = light->position_;

	// Copy UBO to the buffer
	const uint32_t frameIndex = ctx.GetFrameIndex();
	shadowMapUBOBuffers_[frameIndex].UploadBufferData(ctx, ubo, sizeof(ShadowMapUBO));
}

void PipelineShadow::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	uint32_t frameIndex = ctx.GetFrameIndex();
	
	for (uint32_t i = 0; i < ShadowConfig::CascadeCount; ++i)
	{
		renderPass_.BeginRenderPass(
			ctx,
			commandBuffer,
			cascadeFramebuffers_[i].GetFramebuffer(),
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

		PushConstCascadeShadow pc = { .cascadeIndex = i };
		vkCmdPushConstants(
			commandBuffer,
			pipelineLayout_,
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(PushConstCascadeShadow), &pc);

		vkCmdDrawIndirect(
			commandBuffer,
			indirectBuffers_[frameIndex].buffer_,
			0, // offset
			scene_->GetMeshCount(),
			sizeof(VkDrawIndirectCommand));

		vkCmdEndRenderPass(commandBuffer);
	}
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