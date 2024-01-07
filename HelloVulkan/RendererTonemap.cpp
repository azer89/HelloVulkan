#include "RendererTonemap.h"
#include "VulkanUtility.h"
#include "AppSettings.h"

RendererTonemap::RendererTonemap(VulkanDevice& vkDev,
	VulkanImage* singleSampledColorImage) :
	RendererBase(vkDev, nullptr),
	singleSampledColorImage_(singleSampledColorImage)
{
	renderPass_.CreateOnScreenColorOnlyRenderPass(vkDev);

	CreateSwapchainFramebuffers(
		vkDev,
		renderPass_);

	CreateDescriptorPool(
		vkDev,
		0, // uniform
		0, // SSBO
		1, // Texture
		1, // One set per swapchain
		&descriptorPool_);
	CreateDescriptorLayout(vkDev);
	AllocateDescriptorSets(vkDev);
	UpdateDescriptorSets(vkDev);

	CreatePipelineLayout(vkDev.GetDevice(), descriptorSetLayout_, &pipelineLayout_);

	CreateGraphicsPipeline(vkDev,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppSettings::ShaderFolder + "FullscreenTriangle.vert",
			AppSettings::ShaderFolder + "Tonemap.frag",
		},
		&graphicsPipeline_);
}

void RendererTonemap::OnWindowResized(VulkanDevice& vkDev)
{
	RendererBase::OnWindowResized(vkDev);
	UpdateDescriptorSets(vkDev);
}

void RendererTonemap::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t swapchainImageIndex)
{
	renderPass_.BeginRenderPass(vkDev, commandBuffer, swapchainFramebuffers_[swapchainImageIndex]);

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

void RendererTonemap::CreateDescriptorLayout(VulkanDevice& vkDev)
{
	uint32_t bindingIndex = 0u;

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
		&descriptorSetLayout_));
}

void RendererTonemap::AllocateDescriptorSets(VulkanDevice& vkDev)
{
	auto swapChainImageSize = vkDev.GetSwapchainImageCount();

	std::vector<VkDescriptorSetLayout> layouts(swapChainImageSize, descriptorSetLayout_);

	const VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorPool_,
		.descriptorSetCount = static_cast<uint32_t>(swapChainImageSize),
		.pSetLayouts = layouts.data()
	};

	descriptorSets_.resize(swapChainImageSize);

	VK_CHECK(vkAllocateDescriptorSets(vkDev.GetDevice(), &allocInfo, descriptorSets_.data()));
}

void RendererTonemap::UpdateDescriptorSets(VulkanDevice& vkDev)
{
	auto swapChainImageSize = vkDev.GetSwapchainImageCount();
	for (size_t i = 0; i < swapChainImageSize; i++)
	{
		uint32_t bindingIndex = 0u;
		VkDescriptorSet ds = descriptorSets_[i];

		const VkDescriptorImageInfo  imageInfo =
		{
			singleSampledColorImage_->defaultImageSampler_,
			singleSampledColorImage_->imageView_,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		std::vector<VkWriteDescriptorSet> descriptorWrites = {
			ImageWriteDescriptorSet(ds, &imageInfo, bindingIndex++)
		};

		vkUpdateDescriptorSets(
			vkDev.GetDevice(),
			static_cast<uint32_t>(descriptorWrites.size()),
			descriptorWrites.data(),
			0,
			nullptr);
	}
}