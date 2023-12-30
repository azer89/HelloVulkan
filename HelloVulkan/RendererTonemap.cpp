#include "RendererTonemap.h"
#include "VulkanUtility.h"

RendererTonemap::RendererTonemap(VulkanDevice& vkDev,
	VulkanImage* colorImage,
	VulkanImage* depthImage) :
	RendererBase(vkDev, depthImage),
	colorImage_(colorImage_)
{

}

RendererTonemap::~RendererTonemap()
{

}

void RendererTonemap::FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage)
{
	BeginRenderPass(commandBuffer, currentImage);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout_,
		0,
		1,
		&descriptorSets_[currentImage],
		0,
		nullptr);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer);
}

void RendererTonemap::CreateDescriptorLayoutAndSet(VulkanDevice& vkDev)
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

	auto swapChainImageSize = vkDev.GetSwapChainImageSize();

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

	for (size_t i = 0; i < swapChainImageSize; i++)
	{
		bindingIndex = 0u;
		VkDescriptorSet ds = descriptorSets_[i];

		const VkDescriptorImageInfo  imageInfo =
		{
			colorImage_->defaultImageSampler_,
			colorImage_->imageView_,
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