#ifndef RENDERER_BRDF
#define RENDERER_BRDF

#include "RendererBase.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "VulkanUtility.h"

class RendererBRDF final : RendererBase
{
public:
	RendererBRDF(
		VulkanDevice& vkDev, 
		const char* shaderName, 
		uint32_t inputSize, 
		uint32_t outputSize);

	~RendererBRDF();

private:
	VulkanBuffer inBuffer_;
	VulkanBuffer outBuffer_;

	VkDescriptorSet descriptorSet_;

private:
	void CreateComputeDescriptorSet(VkDevice device, VkDescriptorSetLayout descriptorSetLayout);
};

#endif