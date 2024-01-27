#ifndef RENDERER_FINISH
#define RENDERER_FINISH

#include "PipelineBase.h"

class RendererFinish final : public PipelineBase
{
public:
	RendererFinish(VulkanDevice& vkDev);

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;
};

#endif
