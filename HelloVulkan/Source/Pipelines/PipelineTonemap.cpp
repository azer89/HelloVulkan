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
	uint32_t frameIndex = ctx.GetFrameIndex();
	uint32_t swapchainImageIndex = ctx.GetCurrentSwapchainImageIndex();
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
	// Pool
	descriptor_.CreatePool(
		ctx,
		{
			.uboCount_ = 0u,
			.ssboCount_ = 0u,
			.samplerCount_ = 1u,
			.frameCount_ = AppConfig::FrameOverlapCount,
			.setCountPerFrame_ = 1u,
		});

	// Layout
	descriptor_.CreateLayout(ctx,
	{
		{
			.type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.shaderFlags_ = VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = 1
		}
	});

	// Set
	AllocateDescriptorSets(ctx);
	UpdateDescriptorSets(ctx);
}

void PipelineTonemap::AllocateDescriptorSets(VulkanContext& ctx)
{
	auto frameCount = AppConfig::FrameOverlapCount;

	for (size_t i = 0; i < frameCount; i++)
	{
		descriptor_.AllocateSet(ctx, &(descriptorSets_[i]));
	}
}

void PipelineTonemap::UpdateDescriptorSets(VulkanContext& ctx)
{
	VkDescriptorImageInfo imageInfo = singleSampledColorImage_->GetDescriptorImageInfo();

	auto frameCount = AppConfig::FrameOverlapCount;
	for (size_t i = 0; i < frameCount; ++i)
	{
		descriptor_.UpdateSet(
			ctx,
			{
				{.imageInfoPtr_ = &imageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER }
			},
			&(descriptorSets_[i]));
	}
}