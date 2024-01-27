#ifndef PIPELINE_CLEAR
#define PIPELINE_CLEAR

#include "PipelineBase.h"

class PipelineClear final : public PipelineBase
{
public:
	PipelineClear(VulkanDevice& vkDev);
	~PipelineClear();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;
};

#endif
