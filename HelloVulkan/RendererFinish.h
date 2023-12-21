#ifndef RENDERER_FINISH
#define RENDERER_FINISH

#include "RendererBase.h"

class RendererFinish : public RendererBase
{
public:
	RendererFinish(VulkanDevice& vkDev, VulkanImage* depthImage);

	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;
};

#endif
