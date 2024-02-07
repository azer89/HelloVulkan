#ifndef PIPELINE_FINISH
#define PIPELINE_FINISH

#include "PipelineBase.h"

/*
Pipeline to present a swapchain image
*/
class PipelineFinish final : public PipelineBase
{
public:
	PipelineFinish(VulkanDevice& vkDev);

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer) override;
};

#endif
