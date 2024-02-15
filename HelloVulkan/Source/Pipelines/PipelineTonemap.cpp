#include "PipelineTonemap.h"
#include "VulkanUtility.h"
#include "Configs.h"

PipelineTonemap::PipelineTonemap(VulkanDevice& vkDev,
	VulkanImage* singleSampledColorImage) :
	PipelineBase(vkDev,
		{
			.type_ = PipelineType::GraphicsOnScreen
		}),
	// Need to store a pointer for window resizing
	singleSampledColorImage_(singleSampledColorImage)
{
	renderPass_.CreateOnScreenColorOnlyRenderPass(vkDev);

	framebuffer_.Create(vkDev, renderPass_.GetHandle(), {}, IsOffscreen());

	CreateDescriptor(vkDev);

	CreatePipelineLayout(vkDev, descriptor_.layout_, &pipelineLayout_);

	CreateGraphicsPipeline(vkDev,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "FullscreenTriangle.vert",
			AppConfig::ShaderFolder + "Tonemap.frag",
		},
		&pipeline_);
}

void PipelineTonemap::OnWindowResized(VulkanDevice& vkDev)
{
	PipelineBase::OnWindowResized(vkDev);
	UpdateDescriptorSets(vkDev);
}

void PipelineTonemap::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer)
{
	uint32_t frameIndex = vkDev.GetFrameIndex();
	uint32_t swapchainImageIndex = vkDev.GetCurrentSwapchainImageIndex();
	renderPass_.BeginRenderPass(vkDev, commandBuffer, framebuffer_.GetFramebuffer(swapchainImageIndex));
	BindPipeline(vkDev, commandBuffer);
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

void PipelineTonemap::CreateDescriptor(VulkanDevice& vkDev)
{
	// Pool
	descriptor_.CreatePool(
		vkDev,
		{
			.uboCount_ = 0u,
			.ssboCount_ = 0u,
			.samplerCount_ = 1u,
			.frameCount_ = AppConfig::FrameOverlapCount,
			.setCountPerFrame_ = 1u,
		});

	// Layout
	descriptor_.CreateLayout(vkDev,
	{
		{
			.descriptorType_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.shaderFlags_ = VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = 1
		}
	});

	// Set
	AllocateDescriptorSets(vkDev);
	UpdateDescriptorSets(vkDev);
}

void PipelineTonemap::AllocateDescriptorSets(VulkanDevice& vkDev)
{
	auto frameCount = AppConfig::FrameOverlapCount;

	for (size_t i = 0; i < frameCount; i++)
	{
		descriptor_.AllocateSet(vkDev, &(descriptorSets_[i]));
	}
}

void PipelineTonemap::UpdateDescriptorSets(VulkanDevice& vkDev)
{
	VkDescriptorImageInfo imageInfo = singleSampledColorImage_->GetDescriptorImageInfo();

	auto frameCount = AppConfig::FrameOverlapCount;
	for (size_t i = 0; i < frameCount; ++i)
	{
		descriptor_.UpdateSet(
			vkDev,
			{
				{.imageInfoPtr_ = &imageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER }
			},
			&(descriptorSets_[i]));
	}
}