#ifndef PIPELINE_RESOLVE_MULTISAMPLING
#define PIPELINE_RESOLVE_MULTISAMPLING

#include "PipelineBase.h"

/*
This pipeline does not draw anything but it resolves 
a multi-sampled color image to a single-sampled color image
*/
class PipelineResolveMS final : public PipelineBase
{
public:
	PipelineResolveMS(
		VulkanContext& ctx, 
		VulkanImage* multiSampledColorImage, // Input
		VulkanImage* singleSampledColorImage // Output
	);
	~PipelineResolveMS();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;
};

#endif