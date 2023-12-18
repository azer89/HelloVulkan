#include "ComputePBR.h"

enum ComputeDescriptorSetBindingNames : uint32_t
{
	Binding_InputTexture = 0,
	Binding_OutputTexture = 1,
	Binding_OutputMipTail = 2,
};

struct SpecularFilterPushConstants
{
	uint32_t level;
	float roughness;
};

void ComputePBR::CreateSampler(VulkanDevice& vkDev)
{
	VkSamplerCreateInfo createInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

	// Linear, non-anisotropic sampler, wrap address mode (post processing compute shaders)
	createInfo.minFilter = VK_FILTER_LINEAR;
	createInfo.magFilter = VK_FILTER_LINEAR;
	createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	VK_CHECK(vkCreateSampler(vkDev.GetDevice(), &createInfo, nullptr, &sampler_));
}


void ComputePBR::CreateLayouts()
{
	const std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings =
	{
		{ Binding_InputTexture, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, &sampler_ },
		{ Binding_OutputTexture, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
		{ Binding_OutputMipTail, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, numMipmap - 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
	};

	descriptorSetLayout_ = CreateDescriptorSetLayout(&descriptorSetLayoutBindings);
	descriptorSets_ = AllocateDescriptorSet(descriptorPool_, descriptorSetLayout_);

	const std::vector<VkDescriptorSetLayout> pipelineSetLayouts = 
	{
		descriptorSetLayout_,
	};
	const std::vector<VkPushConstantRange> pipelinePushConstantRanges = 
	{
		{ VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SpecularFilterPushConstants) },
	};
	pipelineLayout_ = CreatePipelineLayout(&pipelineSetLayouts, &pipelinePushConstantRanges);
}


void ComputePBR::CreateDescriptorPool()
{
	const std::array<VkDescriptorPoolSize, 2> poolSizes = { {
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, numMipmap },
	} };

	VkDescriptorPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	createInfo.maxSets = 2;
	createInfo.poolSizeCount = (uint32_t)poolSizes.size();
	createInfo.pPoolSizes = poolSizes.data();
	VK_CHECK(vkCreateDescriptorPool(device_, &createInfo, nullptr, &descriptorPool_));
}

VkDescriptorSetLayout ComputePBR::CreateDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>* bindings)
{
	VkDescriptorSetLayout layout;
	VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	if (bindings && bindings->size() > 0)
	{
		createInfo.bindingCount = (uint32_t)bindings->size();
		createInfo.pBindings = bindings->data();
	}
	VK_CHECK(vkCreateDescriptorSetLayout(device_, &createInfo, nullptr, &layout));
	return layout;
}

VkDescriptorSet ComputePBR::AllocateDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout layout)
{
	VkDescriptorSet descriptorSet;
	VkDescriptorSetAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocateInfo.descriptorPool = pool;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &layout;
	VK_CHECK(vkAllocateDescriptorSets(device_, &allocateInfo, &descriptorSet));
	return descriptorSet;
}

VkPipelineLayout ComputePBR::CreatePipelineLayout(
	const std::vector<VkDescriptorSetLayout>* setLayouts,
	const std::vector<VkPushConstantRange>* pushConstants)
{
	VkPipelineLayout layout;
	VkPipelineLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	if (setLayouts && setLayouts->size() > 0)
	{
		createInfo.setLayoutCount = (uint32_t)setLayouts->size();
		createInfo.pSetLayouts = setLayouts->data();
	}
	if (pushConstants && pushConstants->size() > 0)
	{
		createInfo.pushConstantRangeCount = (uint32_t)pushConstants->size();
		createInfo.pPushConstantRanges = pushConstants->data();
	}

	VK_CHECK(vkCreatePipelineLayout(device_, &createInfo, nullptr, &layout));

	return layout;
}