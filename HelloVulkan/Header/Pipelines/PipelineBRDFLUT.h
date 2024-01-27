#ifndef RENDERER_BRDF_LUT
#define RENDERER_BRDF_LUT

#include "PipelineBase.h"
#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "VulkanBuffer.h"
#include "VulkanUtility.h"

class PipelineBRDFLUT final : PipelineBase
{
public:
	PipelineBRDFLUT(VulkanDevice& vkDev);
	~PipelineBRDFLUT();

	void CreateLUT(VulkanDevice& vkDev, VulkanImage* outputLUT);

	void Execute(VulkanDevice& vkDev);

	virtual void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;

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