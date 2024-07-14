#ifndef PIPELINE_BRDF_LUT
#define PIPELINE_BRDF_LUT

#include "PipelineBase.h"
#include "VulkanContext.h"
#include "VulkanImage.h"
#include "VulkanBuffer.h"

/*
Compute pipeline to generate lookup table
*/
class PipelineBRDFLUT final : PipelineBase
{
public:
	PipelineBRDFLUT(VulkanContext& ctx);
	~PipelineBRDFLUT();

	void SetCameraUBO(VulkanContext& ctx, CameraUBO& ubo) override {}
	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;
	void CreateLUT(VulkanContext& ctx, VulkanImage* outputLUT);
	void Execute(VulkanContext& ctx);

private:
	VulkanBuffer outBuffer_{}; // This is the lookup table which has to be transferred to an image
	VkDescriptorSet descriptorSet_{};

private:
	void CreateDescriptor(VulkanContext& ctx);
};

#endif