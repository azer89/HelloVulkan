#ifndef PIPELINE_CLEAR
#define PIPELINE_CLEAR

#include "PipelineBase.h"

/*
Clear a swapchain image
*/
class PipelineClear final : public PipelineBase
{
public:
	PipelineClear(VulkanDevice& vkDev);
	~PipelineClear();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer) override;
};

#endif
