#ifndef PIPELINE_RESOLVE_MULTISAMPLING
#define PIPELINE_RESOLVE_MULTISAMPLING

#include "PipelineBase.h"
#include "ResourcesShared.h"

/*
This pipeline does not draw anything but it resolves 
a multi-sampled color image to a single-sampled color image
*/
class PipelineResolveMS final : public PipelineBase
{
public:
	PipelineResolveMS(
		VulkanContext& ctx,
		ResourcesShared* resourcesShared
	);
	~PipelineResolveMS();

	void SetCameraUBO(VulkanContext& ctx, CameraUBO& ubo) override {}
	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;
};

#endif