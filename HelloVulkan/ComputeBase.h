#ifndef COMPUTE_BASE
#define COMPUTE_BASE

#include "VulkanDevice.h"
#include "VulkanBuffer.h"

class ComputeBase
{
public:
	ComputeBase(
		VulkanDevice& vkDev, 
		const char* shaderName, 
		uint32_t inputSize, 
		uint32_t outputSize);

	virtual ~ComputeBase();

public:
	VulkanDevice& vkDev_;

	VulkanBuffer inBuffer_;
	VulkanBuffer outBuffer_;

	VkDescriptorSetLayout descriptorLayout_;
	VkPipelineLayout pipelineLayout_;
	VkPipeline pipeline_;

	VkDescriptorPool descriptorPool_;
	VkDescriptorSet descriptorSet_;
};

#endif