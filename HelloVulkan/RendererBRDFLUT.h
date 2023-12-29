#ifndef RENDERER_BRDF
#define RENDERER_BRDF

#include "RendererBase.h"
#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "VulkanBuffer.h"
#include "VulkanUtility.h"

class RendererBRDFLUT final : RendererBase
{
public:
	RendererBRDFLUT(VulkanDevice& vkDev);

	virtual ~RendererBRDFLUT();

	void CreateLUT(VulkanDevice& vkDev, VulkanImage* outputLUT);

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