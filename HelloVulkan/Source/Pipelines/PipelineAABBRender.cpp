#include "PipelineAABBRender.h"
#include "ResourcesShared.h"
#include "Scene.h"

PipelineAABBRender::PipelineAABBRender(
	VulkanContext& ctx,
	ResourcesShared* resShared,
	Scene* scene,
	uint8_t renderBit) :
	PipelineBase(ctx,
	{
		.type_ = PipelineType::GraphicsOffScreen,
		.msaaSamples_ = resShared->multiSampledColorImage_.multisampleCount_,
		.depthTest_ = true,
		.depthWrite_ = false
	}),
	scene_(scene),
	shouldRender_(false)
{
	VulkanBuffer::CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(CameraUBO), AppConfig::FrameCount);
	renderPass_.CreateOffScreenRenderPass(ctx, renderBit, config_.msaaSamples_);
	framebuffer_.CreateResizeable(
		ctx,
		renderPass_.GetHandle(),
		{
			&(resShared->multiSampledColorImage_),
			&(resShared->depthImage_)
		},
		IsOffscreen()
	);
	CreateDescriptor(ctx);
	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_);
	CreateGraphicsPipeline(ctx,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "AABB/AABBRender.vert",
			AppConfig::ShaderFolder + "AABB/AABBRender.frag",
		},
		&pipeline_
	);
}

PipelineAABBRender::~PipelineAABBRender()
{
}

void PipelineAABBRender::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	if (!shouldRender_)
	{
		return;
	}

	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "AABB", tracy::Color::Orange4);

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
	const uint32_t boxCount = scene_->GetInstanceCount();
	ctx.InsertDebugLabel(commandBuffer, "PipelineAABBRender", 0xff99ff99);
	vkCmdDraw(commandBuffer, 36, boxCount, 0, 0);
	vkCmdEndRenderPass(commandBuffer);
}

void PipelineAABBRender::CreateDescriptor(VulkanContext& ctx)
{
	constexpr uint32_t frameCount = AppConfig::FrameCount;

	VulkanDescriptorInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // 0
	dsInfo.AddBuffer(&(scene_->transformedBoundingBoxBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // 1

	// Create pool and layout
	descriptor_.CreatePoolAndLayout(ctx, dsInfo, frameCount, 1u);

	for (size_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(cameraUBOBuffers_[i]), 0);
		descriptor_.CreateSet(ctx, dsInfo, &(descriptorSets_[i]));
	}
}