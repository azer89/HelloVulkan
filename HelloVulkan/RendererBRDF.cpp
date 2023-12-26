#include "RendererBRDF.h"
#include "VulkanShader.h"


RendererBRDF::RendererBRDF(
	VulkanDevice& vkDev,
	const char* shaderName,
	uint32_t inputSize,
	uint32_t outputSize) :
	RendererBase(vkDev, {})
{
	inBuffer_.CreateSharedBuffer(vkDev, inputSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	outBuffer_.CreateSharedBuffer(vkDev, outputSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VulkanShader shader;
	shader.Create(vkDev.GetDevice(), shaderName);

	CreateComputeDescriptorSetLayout(vkDev.GetDevice());
	CreatePipelineLayout(vkDev.GetDevice(), descriptorSetLayout_, &pipelineLayout_);
	CreateComputePipeline(vkDev.GetDevice(), shader.GetShaderModule());
	CreateComputeDescriptorSet(vkDev.GetDevice(), descriptorSetLayout_);

	shader.Destroy(vkDev.GetDevice());
}

RendererBRDF::~RendererBRDF()
{
	inBuffer_.Destroy(device_);
	outBuffer_.Destroy(device_);

	vkDestroyPipeline(device_, pipeline_, nullptr);
}

void RendererBRDF::FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage)
{

}

void RendererBRDF::CreateComputeDescriptorSetLayout(VkDevice device)
{
	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[2] =
	{
		{ 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0 },
		{ 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0 }
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo =
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		0,
		0,
		2,
		descriptorSetLayoutBindings
	};

	VK_CHECK(vkCreateDescriptorSetLayout(
		device,
		&descriptorSetLayoutCreateInfo,
		0,
		&descriptorSetLayout_));
}

void RendererBRDF::CreateComputeDescriptorSet(VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
{
	// Descriptor pool
	VkDescriptorPoolSize descriptorPoolSize = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 };

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, 0, 0, 1, 1, &descriptorPoolSize
	};

	VK_CHECK(vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, 0, &descriptorPool_));

	// Descriptor set
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		0, descriptorPool_, 1, &descriptorSetLayout_
	};

	VK_CHECK(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet_));

	// Finally, update descriptor set with concrete buffer pointers
	VkDescriptorBufferInfo inBufferInfo = { inBuffer_.buffer_, 0, VK_WHOLE_SIZE };

	VkDescriptorBufferInfo outBufferInfo = { outBuffer_.buffer_, 0, VK_WHOLE_SIZE };

	VkWriteDescriptorSet writeDescriptorSet[2] = {
		{ 
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, 
			0, 
			descriptorSet_, 
			0, 
			0, 
			1, 
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
			0,  
			&inBufferInfo, 0 },
		{ 
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, 
			0, 
			descriptorSet_, 
			1, 
			0, 
			1, 
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
			0, 
			&outBufferInfo, 
			0 
		}
	};

	vkUpdateDescriptorSets(device, 2, writeDescriptorSet, 0, 0);
}

void RendererBRDF::CreateComputePipeline(
	VkDevice device,
	VkShaderModule computeShader)
{
	VkComputePipelineCreateInfo computePipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = {  // ShaderStageInfo, just like in graphics pipeline, but with a single COMPUTE stage
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = computeShader,
			.pName = "main",
			/* we don't use specialization */
			.pSpecializationInfo = nullptr
		},
		.layout = pipelineLayout_,
		.basePipelineHandle = 0,
		.basePipelineIndex = 0
	};

	/* no caching, single pipeline creation*/
	VK_CHECK(vkCreateComputePipelines(device, 0, 1, &computePipelineCreateInfo, nullptr, &pipeline_));
}