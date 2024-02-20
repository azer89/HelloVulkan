#include "PipelineSkybox.h"
#include "VulkanUtility.h"
#include "Configs.h"
#include "UBO.h"

#include "glm/glm.hpp"

#include <cmath>
#include <array>

PipelineSkybox::PipelineSkybox(VulkanContext& ctx, 
	VulkanImage* envMap,
	VulkanImage* depthImage,
	VulkanImage* offscreenColorImage,
	uint8_t renderBit) :
	PipelineBase(ctx,
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.msaaSamples_ = offscreenColorImage->multisampleCount_,
			.vertexBufferBind_ = false,
			.depthTest_ = true,
			.depthWrite_ = false // Do not write to depth image
		}),
	envCubemap_(envMap)
{
	CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(CameraUBO), AppConfig::FrameOverlapCount);

	// Note that this pipeline is offscreen rendering
	renderPass_.CreateOffScreenRenderPass(ctx, renderBit, config_.msaaSamples_);
	framebuffer_.CreateResizeable(
		ctx,
		renderPass_.GetHandle(),
		{
			offscreenColorImage,
			depthImage
		},
		IsOffscreen()
	);

	CreateDescriptor(ctx);
	
	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_);

	CreateGraphicsPipeline(ctx,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "Cube.vert",
			AppConfig::ShaderFolder + "Cube.frag",
		},
		&pipeline_
		); 
}

PipelineSkybox::~PipelineSkybox()
{
}

void PipelineSkybox::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	uint32_t frameIndex = ctx.GetFrameIndex();
	renderPass_.BeginRenderPass(ctx, commandBuffer, framebuffer_.GetFramebuffer());
	BindPipeline(ctx, commandBuffer);
	vkCmdBindDescriptorSets(
		commandBuffer, 
		VK_PIPELINE_BIND_POINT_GRAPHICS, 
		pipelineLayout_, 
		0, 
		1, 
		&descriptorSets_[frameIndex],
		0, 
		nullptr);
	vkCmdDraw(commandBuffer, 36, 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);
}

void PipelineSkybox::CreateDescriptor(VulkanContext& ctx)
{
	// Pool
	descriptor_.CreatePool(
		ctx,
		{
			.uboCount_ = 1u,
			.ssboCount_ = 0u,
			.samplerCount_ = 1u,
			.frameCount_ = AppConfig::FrameOverlapCount,
			.setCountPerFrame_ = 1u,
		});

	// Layout
	descriptor_.CreateLayout(ctx,
	{
		{
			.descriptorType_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT,
			.bindingCount_ = 1
		},
		{
			.descriptorType_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.shaderFlags_ = VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = 1
		}
	});

	// Set
	auto frameCount = AppConfig::FrameOverlapCount;

	VkDescriptorImageInfo imageInfo = envCubemap_->GetDescriptorImageInfo();

	for (size_t i = 0; i < frameCount; i++)
	{
		VkDescriptorBufferInfo bufferInfo = { cameraUBOBuffers_[i].buffer_, 0, sizeof(CameraUBO) };

		descriptor_.CreateSet(
			ctx,
			{
				{.bufferInfoPtr_ = &bufferInfo, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
				{.imageInfoPtr_ = &imageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER }
			},
			&(descriptorSets_[i]));
	}
}