#ifndef PIPELINE_CLEAR
#define PIPELINE_CLEAR

#include "PipelineBase.h"

/*
Clear a swapchain image
*/
class PipelineClear final : public PipelineBase
{
public:
	PipelineClear(VulkanContext& vkDev);
	~PipelineClear();

	void FillCommandBuffer(VulkanContext& vkDev, VkCommandBuffer commandBuffer) override;
};

#endif
