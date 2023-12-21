#ifndef RENDERER_CLEAR
#define RENDERER_CLEAR

#include "RendererBase.h"

class RendererClear final : public RendererBase
{
public:
	RendererClear(VulkanDevice& vkDev, VulkanImage* depthImage);

	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	bool shouldClearDepth_;
};

#endif
