#ifndef PIPELINE_CLEAR
#define PIPELINE_CLEAR

#include "PipelineBase.h"

/*
Clear a swapchain image
*/
class PipelineClear final : public PipelineBase
{
public:
	PipelineClear(VulkanContext& ctx);
	~PipelineClear();

	void SetCameraUBO(VulkanContext& ctx, CameraUBO& ubo) override {}
	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;
};

#endif
