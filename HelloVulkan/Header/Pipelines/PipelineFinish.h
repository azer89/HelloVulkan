#ifndef PIPELINE_FINISH
#define PIPELINE_FINISH

#include "PipelineBase.h"

/*
Pipeline to present a swapchain image
*/
class PipelineFinish final : public PipelineBase
{
public:
	PipelineFinish(VulkanContext& ctx);

	void SetCameraUBO(VulkanContext& ctx, CameraUBO& ubo) override {}
	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;
};

#endif
