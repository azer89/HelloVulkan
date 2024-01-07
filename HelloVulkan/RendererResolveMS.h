#ifndef RENDERER_RESOLVE_MULTISAMPLING
#define RENDERER_RESOLVE_MULTISAMPLING

#include "RendererBase.h"

/*
Class that resolves a multi-sampled color image to a single-sampled color image
*/
class RendererResolveMS final : public RendererBase
{
public:
	RendererResolveMS(
		VulkanDevice& vkDev, 
		VulkanImage* multiSampledColorImage, // Input
		VulkanImage* singleSampledColorImage // Output
	);
	~RendererResolveMS();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;

	void OnWindowResized(VulkanDevice& vkDev) override;

private:
	VulkanImage* multiSampledColorImage_; // Input
	VulkanImage* singleSampledColorImage_; // Output
};

#endif