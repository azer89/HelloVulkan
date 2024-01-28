#ifndef PIPELINE_BRDF_LUT
#define PIPELINE_BRDF_LUT

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
	// This is the lookup table
	VulkanBuffer outBuffer_;

	VkDescriptorSet descriptorSet_;

private:
	void SetupDescriptor(VulkanDevice& vkDev);
};

#endif