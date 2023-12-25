#include "RendererBRDF.h"


RendererBRDF::RendererBRDF(
	VulkanDevice& vkDev,
	const char* shaderName,
	uint32_t inputSize,
	uint32_t outputSize) :
	RendererBase(vkDev, {})
{

}

RendererBRDF::~RendererBRDF()
{

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