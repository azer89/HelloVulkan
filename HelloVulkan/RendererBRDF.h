#ifndef RENDERER_BRDF
#define RENDERER_BRDF

#include "RendererBase.h"
#include "VulkanDevice.h"
#include "VulkanTexture.h"
#include "VulkanBuffer.h"
#include "VulkanUtility.h"

class RendererBRDF final : RendererBase
{
public:
	RendererBRDF(VulkanDevice& vkDev);

	virtual ~RendererBRDF();

	void CreateLUT(VulkanDevice& vkDev, VulkanTexture* outputLUT);

	void Execute(VulkanDevice& vkDev);

	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	VulkanBuffer inBuffer_;
	VulkanBuffer outBuffer_;

	VkDescriptorSet descriptorSet_;

	VkPipeline pipeline_;

private:
	void CreateComputeDescriptorSetLayout(VkDevice device);
	
	void CreateComputeDescriptorSet(VkDevice device, VkDescriptorSetLayout descriptorSetLayout);

	void CreateComputePipeline(
		VkDevice device,
		VkShaderModule computeShader);

};

#endif