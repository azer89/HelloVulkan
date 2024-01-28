#include "PipelineTonemap.h"
#include "VulkanUtility.h"
#include "Configs.h"

PipelineTonemap::PipelineTonemap(VulkanDevice& vkDev,
	VulkanImage* singleSampledColorImage) :
	PipelineBase(vkDev,
		{
			.flags_ = PipelineFlags::GraphicsOnScreen
		}),
	singleSampledColorImage_(singleSampledColorImage)
{
	renderPass_.CreateOnScreenColorOnlyRenderPass(vkDev);

	framebuffer_.Create(vkDev, renderPass_.GetHandle(), {}, IsOffscreen());

	descriptor_.CreatePool(
		vkDev,
		{
			.uboCount_ = 0u,
			.ssboCount_ = 0u,
			.samplerCount_ = 1u,
			.swapchainCount_ = static_cast<uint32_t>(vkDev.GetSwapchainImageCount()),
			.setCountPerSwapchain_ = 1u,
		});

	CreateDescriptorLayout(vkDev);
	AllocateDescriptorSets(vkDev);
	UpdateDescriptorSets(vkDev);

	CreatePipelineLayout(vkDev.GetDevice(), descriptor_.layout_, &pipelineLayout_);

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

void PipelineTonemap::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t swapchainImageIndex)
{
	renderPass_.BeginRenderPass(vkDev, commandBuffer, framebuffer_.GetFramebuffer(swapchainImageIndex));

	BindPipeline(vkDev, commandBuffer);
	
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout_,
		0,
		1,
		&descriptorSets_[swapchainImageIndex],
		0,
		nullptr);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer);
}

void PipelineTonemap::CreateDescriptorLayout(VulkanDevice& vkDev)
{
	descriptor_.CreateLayout(vkDev,
	{
		{
			.descriptorType_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.shaderFlags_ = VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = 1
		}
	});
}

void PipelineTonemap::AllocateDescriptorSets(VulkanDevice& vkDev)
{
	auto swapChainImageSize = vkDev.GetSwapchainImageCount();
	descriptorSets_.resize(swapChainImageSize);

	for (size_t i = 0; i < swapChainImageSize; i++)
	{
		descriptor_.AllocateSet(vkDev, &(descriptorSets_[i]));
	}
}

void PipelineTonemap::UpdateDescriptorSets(VulkanDevice& vkDev)
{
	VkDescriptorImageInfo imageInfo = singleSampledColorImage_->GetDescriptorImageInfo();

	auto swapChainImageSize = vkDev.GetSwapchainImageCount();
	for (size_t i = 0; i < swapChainImageSize; ++i)
	{
		descriptor_.UpdateSet(
			vkDev,
			{
				{.imageInfoPtr_ = &imageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER }
			},
			&(descriptorSets_[i]));
	}
}