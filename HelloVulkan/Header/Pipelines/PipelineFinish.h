#ifndef PIPELINE_FINISH
#define PIPELINE_FINISH

#include "PipelineBase.h"

class PipelineFinish final : public PipelineBase
{
public:
	PipelineFinish(VulkanDevice& vkDev);

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;
};

#endif
