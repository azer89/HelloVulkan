#include "PipelineTonemap.h"
#include "VulkanUtility.h"
#include "Configs.h"

PipelineTonemap::PipelineTonemap(VulkanDevice& vkDev,
	VulkanImage* singleSampledColorImage) :
	PipelineBase(vkDev, PipelineFlags::GraphicsOnScreen),
	singleSampledColorImage_(singleSampledColorImage)
{
	renderPass_.CreateOnScreenColorOnlyRenderPass(vkDev);

	framebuffer_.Create(vkDev, renderPass_.GetHandle(), {}, IsOffscreen());

	/*CreateDescriptorPool(
		vkDev,
		0, // uniform
		0, // SSBO
		1, // Texture
		1, // One set per swapchain
		&descriptorPool_);*/
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
	/*uint32_t bindingIndex = 0u;

	std::vector<VkDescriptorSetLayoutBinding> bindings =
	{
		DescriptorSetLayoutBinding(
			bindingIndex++,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT)
	};

	const VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data()
	};

	VK_CHECK(vkCreateDescriptorSetLayout(
		vkDev.GetDevice(),
		&layoutInfo,
		nullptr,
		&descriptorSetLayout_));*/
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
	/*auto swapChainImageSize = vkDev.GetSwapchainImageCount();

	std::vector<VkDescriptorSetLayout> layouts(swapChainImageSize, descriptorSetLayout_);

	const VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorPool_,
		.descriptorSetCount = static_cast<uint32_t>(swapChainImageSize),
		.pSetLayouts = layouts.data()
	};

	descriptorSets_.resize(swapChainImageSize);

	VK_CHECK(vkAllocateDescriptorSets(vkDev.GetDevice(), &allocInfo, descriptorSets_.data()));*/
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

		/*uint32_t bindingIndex = 0u;
		VkDescriptorSet ds = descriptorSets_[i];

		std::vector<VkWriteDescriptorSet> descriptorWrites = {
			ImageWriteDescriptorSet(ds, &imageInfo, bindingIndex++)
		};

		vkUpdateDescriptorSets(
			vkDev.GetDevice(),
			static_cast<uint32_t>(descriptorWrites.size()),
			descriptorWrites.data(),
			0,
			nullptr);*/
	}
}