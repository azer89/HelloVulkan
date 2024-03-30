#include "PipelineInfiniteGrid.h"
#include "ResourcesShared.h"

PipelineInfiniteGrid::PipelineInfiniteGrid(
	VulkanContext& ctx,
	ResourcesShared* resourcesShared,
	float yPosition,
	uint8_t renderBit) :
	PipelineBase(ctx,
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.msaaSamples_ = resourcesShared->multiSampledColorImage_.multisampleCount_,
			.depthTest_ = true,
			.depthWrite_ = false // Do not write to depth image
		}),
	yPosition_(yPosition),
	shouldRender_(true)
{
	VulkanBuffer::CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(CameraUBO), AppConfig::FrameCount);
	renderPass_.CreateOffScreenRenderPass(ctx, renderBit, config_.msaaSamples_);
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
	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_, sizeof(float), VK_SHADER_STAGE_VERTEX_BIT);
	CreateGraphicsPipeline(ctx,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "InfiniteGrid/Grid.vert",
			AppConfig::ShaderFolder + "InfiniteGrid/Grid.frag",
		},
		&pipeline_
	);
}

PipelineInfiniteGrid::~PipelineInfiniteGrid()
{

}

void PipelineInfiniteGrid::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	if (!shouldRender_)
	{
		return;
	}
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "InfiniteGrid", tracy::Color::Lime);
	const uint32_t frameIndex = ctx.GetFrameIndex();
	renderPass_.BeginRenderPass(ctx, commandBuffer, framebuffer_.GetFramebuffer());
	BindPipeline(ctx, commandBuffer);
	vkCmdPushConstants(
		commandBuffer,
		pipelineLayout_,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(float), &yPosition_);
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout_,
		0,
		1,
		&descriptorSets_[frameIndex],
		0,
		nullptr);
	ctx.InsertDebugLabel(commandBuffer, "PipelineInfiniteGrid", 0xff99ff99);
	vkCmdDraw(commandBuffer, 6, 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);
}

void PipelineInfiniteGrid::CreateDescriptor(VulkanContext& ctx)
{
	constexpr uint32_t frameCount = AppConfig::FrameCount;
	VulkanDescriptorInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	descriptor_.CreatePoolAndLayout(ctx, dsInfo, frameCount, 1u);
	for (size_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(cameraUBOBuffers_[i]), 0);
		descriptor_.CreateSet(ctx, dsInfo, &(descriptorSets_[i]));
	}
}
