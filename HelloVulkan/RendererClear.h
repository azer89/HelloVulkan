#ifndef RENDERER_CLEAR
#define RENDERER_CLEAR

#include "RendererBase.h"

class RendererClear : public RendererBase
{
public:
	RendererClear(VulkanDevice& vkDev, VulkanImage depthTexture);

	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	bool shouldClearDepth;
};

#endif
