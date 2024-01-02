#ifndef RENDERER_RESOLVE_MULTISAMPLING
#define RENDERER_RESOLVE_MULTISAMPLING

#include "RendererBase.h"

class RendererResolveMultisampling final : public RendererBase
{
public:
	RendererResolveMultisampling(
		VulkanDevice& vkDev, 
		VulkanImage* multiSampledColorImage, // Input
		VulkanImage* singleSampledColorImage // Output
	);

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;
};

#endif