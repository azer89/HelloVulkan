#ifndef RENDERER_RESOLVE_MULTISAMPLING
#define RENDERER_RESOLVE_MULTISAMPLING

#include "PipelineBase.h"

/*
Class that resolves a multi-sampled color image to a single-sampled color image
*/
class PipelineResolveMS final : public PipelineBase
{
public:
	PipelineResolveMS(
		VulkanDevice& vkDev, 
		VulkanImage* multiSampledColorImage, // Input
		VulkanImage* singleSampledColorImage // Output
	);
	~PipelineResolveMS();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	VulkanImage* multiSampledColorImage_; // Input
	VulkanImage* singleSampledColorImage_; // Output
};

#endif