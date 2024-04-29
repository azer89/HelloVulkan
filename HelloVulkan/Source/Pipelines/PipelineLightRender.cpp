#include "PipelineLightRender.h"
#include "ResourcesLight.h"
#include "ResourcesShared.h"
#include "Configs.h"

#include <array>

PipelineLightRender::PipelineLightRender(
	VulkanContext& ctx,
	ResourcesLight* resourcesLights,
	ResourcesShared* resourcesShared,
	uint8_t renderBit) :
	PipelineBase(ctx, 
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.msaaSamples_ = resourcesShared->multiSampledColorImage_.multisampleCount_,
			.depthTest_ = true,
			.depthWrite_ = false // To "blend" the circles
		}
	), // Offscreen rendering
	resLight_(resourcesLights),
	shouldRender_(true)
{
	VulkanBuffer::CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(CameraUBO), AppConfig::FrameCount);
	renderPass_.CreateOffScreen(ctx, renderBit, config_.msaaSamples_);
	framebuffer_.CreateResizeable(
		ctx,
		renderPass_.GetHandle(),
		{
			&(resourcesShared->multiSampledColorImage_),
			&(resourcesShared->depthImage_)
		},
		IsOffscreen()
	);
	CreateDescriptor(ctx);
	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_);
	CreateGraphicsPipeline(ctx,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "Common/LightOrb.vert",
			AppConfig::ShaderFolder + "Common/LightOrb.frag",
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

	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "Lights", tracy::Color::AliceBlue);
	const uint32_t frameIndex = ctx.GetFrameIndex();
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
	ctx.InsertDebugLabel(commandBuffer, "PipelineLightRender", 0xff99ff99);
	vkCmdDraw(
		commandBuffer, 
		6, // Draw a quad
		resLight_->GetLightCount(), 
		0, 
		0);
	vkCmdEndRenderPass(commandBuffer);
}

void PipelineLightRender::CreateDescriptor(VulkanContext& ctx)
{
	constexpr uint32_t frameCount = AppConfig::FrameCount;

	VulkanDescriptorSetInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT); // 0
	dsInfo.AddBuffer(resLight_->GetVulkanBufferPtr(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // 1

	// Create pool and layout
	descriptor_.CreatePoolAndLayout(ctx, dsInfo, frameCount, 1u);

	// Create sets
	for (size_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(cameraUBOBuffers_[i]), 0);
		descriptor_.CreateSet(ctx, dsInfo, &(descriptorSets_[i]));
	}
}