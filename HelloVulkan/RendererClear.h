#ifndef RENDERER_CLEAR
#define RENDERER_CLEAR

#include "RendererBase.h"

class RendererClear final : public RendererBase
{
public:
	RendererClear(VulkanDevice& vkDev, VulkanImage* depthImage);

	void RecordCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;
};

#endif
