#ifndef RENDERER_CLEAR
#define RENDERER_CLEAR

#include "PipelineBase.h"

class RendererClear final : public RendererBase
{
public:
	RendererClear(VulkanDevice& vkDev);
	~RendererClear();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;
};

#endif
