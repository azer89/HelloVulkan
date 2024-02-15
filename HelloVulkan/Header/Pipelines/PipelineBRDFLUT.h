#ifndef PIPELINE_BRDF_LUT
#define PIPELINE_BRDF_LUT

#include "PipelineBase.h"
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "VulkanBuffer.h"
#include "VulkanUtility.h"

/*
Compute pipeline to generate lookup table
*/
class PipelineBRDFLUT final : PipelineBase
{
public:
	PipelineBRDFLUT(VulkanContext& vkDev);
	~PipelineBRDFLUT();

	void CreateLUT(VulkanContext& vkDev, VulkanImage* outputLUT);

	void Execute(VulkanContext& vkDev);

	virtual void FillCommandBuffer(VulkanContext& vkDev, VkCommandBuffer commandBuffer) override;

private:
	// This is the lookup table which has to be transferred to an image
	VulkanBuffer outBuffer_;

	VkDescriptorSet descriptorSet_;

private:
	void CreateDescriptor(VulkanContext& vkDev);
};

#endif