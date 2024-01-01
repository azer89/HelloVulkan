#ifndef RENDERER_FINISH
#define RENDERER_FINISH

#include "RendererBase.h"

class RendererFinish final : public RendererBase
{
public:
	RendererFinish(VulkanDevice& vkDev, VulkanImage* depthImage);

	void RecordCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;
};

#endif
