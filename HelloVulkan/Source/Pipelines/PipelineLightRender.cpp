#include "PipelineLightRender.h"
#include "VulkanUtility.h"

#include "Configs.h"

#include <array>

PipelineLightRender::PipelineLightRender(
	VulkanContext& ctx,
	Lights* lights,
	VulkanImage* depthImage, 
	VulkanImage* offscreenColorImage,
	uint8_t renderBit) :
	PipelineBase(ctx, 
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.msaaSamples_ = offscreenColorImage->multisampleCount_,
			.depthTest_ = true,
			.depthWrite_ = false // To "blend" the circles
		}
	), // Offscreen rendering
	lights_(lights),
	shouldRender_(true)
{
	CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(CameraUBO), AppConfig::FrameOverlapCount);

	renderPass_.CreateOffScreenRenderPass(ctx, renderBit, config_.msaaSamples_);

	framebuffer_.Create(
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
			AppConfig::ShaderFolder + "LightCircle.vert",
			AppConfig::ShaderFolder + "LightCircle.frag",
		},
		&pipeline_
		);
}

PipelineLightRender::~PipelineLightRender()
{
}

void PipelineLightRender::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	if (!shouldRender_)
	{
		return;
	}

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
	vkCmdDraw(
		commandBuffer, 
		6, // Draw a quad
		lights_->GetLightCount(), 
		0, 
		0);
	vkCmdEndRenderPass(commandBuffer);
}

void PipelineLightRender::CreateDescriptor(VulkanContext& ctx)
{
	// Pool
	descriptor_.CreatePool(
		ctx,
		{
			.uboCount_ = 1u,
			.ssboCount_ = 1u,
			.samplerCount_ = 0u,
			.frameCount_ = AppConfig::FrameOverlapCount,
			.setCountPerFrame_ = 1u,
		});

	// Layout
	descriptor_.CreateLayout(ctx,
	{
		{
			.descriptorType_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = 1
		},
		{
			.descriptorType_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT,
			.bindingCount_ = 1
		}
	});

	// Set
	size_t frameCount = AppConfig::FrameOverlapCount;

	for (size_t i = 0; i < frameCount; ++i)
	{
		VkDescriptorBufferInfo bufferInfo1 = {.buffer = cameraUBOBuffers_[i].buffer_, .offset = 0, .range = sizeof(CameraUBO)};
		VkDescriptorBufferInfo bufferInfo2 = {.buffer = lights_->GetSSBOBuffer(), .offset = 0, .range = lights_->GetSSBOSize()};

		descriptor_.CreateSet(
			ctx, 
			{
				{.bufferInfoPtr_ = &bufferInfo1, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
				{.bufferInfoPtr_ = &bufferInfo2, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }
			}, 
			&(descriptorSets_[i]));
	}
}