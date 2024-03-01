#include "PipelineTonemap.h"
#include "VulkanUtility.h"
#include "Configs.h"

PipelineTonemap::PipelineTonemap(VulkanContext& ctx,
	VulkanImage* singleSampledColorImage) :
	PipelineBase(ctx,
		{
			.type_ = PipelineType::GraphicsOnScreen
		}),
	// Need to store a pointer for window resizing
	singleSampledColorImage_(singleSampledColorImage)
{
	renderPass_.CreateOnScreenColorOnlyRenderPass(ctx);

	framebuffer_.CreateResizeable(ctx, renderPass_.GetHandle(), {}, IsOffscreen());

	CreateDescriptor(ctx);

	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_);

	CreateGraphicsPipeline(ctx,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "FullscreenTriangle.vert",
			AppConfig::ShaderFolder + "Tonemap.frag",
		},
		&pipeline_);
}

void PipelineTonemap::OnWindowResized(VulkanContext& ctx)
{
	PipelineBase::OnWindowResized(ctx);
	UpdateDescriptorSets(ctx);
}

void PipelineTonemap::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	const uint32_t frameIndex = ctx.GetFrameIndex();
	const uint32_t swapchainImageIndex = ctx.GetCurrentSwapchainImageIndex();
	renderPass_.BeginRenderPass(ctx, commandBuffer, framebuffer_.GetFramebuffer(swapchainImageIndex));
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
	vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);
}

void PipelineTonemap::CreateDescriptor(VulkanContext& ctx)
{
	constexpr uint32_t frameCount = AppConfig::FrameOverlapCount;

	descriptorBuildInfo_.AddImage(nullptr);

	// Pool and layout
	descriptor_.CreatePoolAndLayout(ctx, descriptorBuildInfo_, frameCount, 1u);

	// Sets
	AllocateDescriptorSets(ctx);
	UpdateDescriptorSets(ctx);
}

void PipelineTonemap::AllocateDescriptorSets(VulkanContext& ctx)
{
	constexpr auto frameCount = AppConfig::FrameOverlapCount;

	for (size_t i = 0; i < frameCount; i++)
	{
		descriptor_.AllocateSet(ctx, &(descriptorSets_[i]));
	}
}

void PipelineTonemap::UpdateDescriptorSets(VulkanContext& ctx)
{
	constexpr auto frameCount = AppConfig::FrameOverlapCount;
	descriptorBuildInfo_.UpdateImage(singleSampledColorImage_, 0);
	for (size_t i = 0; i < frameCount; ++i)
	{
		descriptor_.UpdateSet(ctx, descriptorBuildInfo_, &(descriptorSets_[i]));
	}
}