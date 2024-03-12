#include "PipelineLine.h"
#include "ResourcesShared.h"
#include "Scene.h"

PipelineLine::PipelineLine(
	VulkanContext& ctx,
	ResourcesShared* resShared,
	Scene* scene,
	uint8_t renderBit) :
	PipelineBase(ctx,
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.topology_ = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
			.vertexBufferBind_ = false,
			.depthTest_ = true,
			.depthWrite_ = false // Do not write to depth image
		}),
	scene_(scene),
	shouldRender_(false)
{
	CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(CameraUBO), AppConfig::FrameOverlapCount);
	renderPass_.CreateOffScreenRenderPass(ctx, renderBit);
	framebuffer_.CreateResizeable(
		ctx,
		renderPass_.GetHandle(),
		{
			&(resShared->multiSampledColorImage_),
			&(resShared->depthImage_)
		},
		IsOffscreen()
	);

	// TODO
	/*
	CreateLines(ctx);
	CreateDescriptor(ctx);
	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_);
	CreateGraphicsPipeline(ctx,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "Line.vert",
			AppConfig::ShaderFolder + "Line.frag",
		},
		&pipeline_
		);*/
}

PipelineLine::~PipelineLine()
{
	lineBuffer_.Destroy();
}

void PipelineLine::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	if (!shouldRender_)
	{
		return;
	}
}

void PipelineLine::CreateLines(VulkanContext& ctx)
{
}

void PipelineLine::CreateDescriptor(VulkanContext& ctx)
{
}