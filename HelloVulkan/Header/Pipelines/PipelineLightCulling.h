#ifndef PIPELINE_LIGHT_CULLING
#define PIPELINE_LIGHT_CULLING

#include "PipelineBase.h"
#include "VulkanContext.h"
#include "ResourcesClusterForward.h"
#include "ResourcesLight.h"
#include "Configs.h"

#include <array>

/*
Clustered Forward
*/
class PipelineLightCulling final : public PipelineBase
{
public:
	PipelineLightCulling(VulkanContext& ctx, ResourcesLight* resourcesLight, ResourcesClusterForward* resourcesCF);
	~PipelineLightCulling();

	void SetCameraUBO(VulkanContext& ctx, CameraUBO& ubo) override {}
	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;
	void ResetGlobalIndex(VulkanContext& ctx);
	void SetClusterForwardUBO(VulkanContext& ctx, ClusterForwardUBO& ubo);

private:
	ResourcesLight* resourcesLight_;
	ResourcesClusterForward* resourcesCF_;

	std::vector<VulkanBuffer> cfUBOBuffers_;
	std::array<VkDescriptorSet, AppConfig::FrameCount> descriptorSets_;

private:
	void Execute(VulkanContext& ctx, VkCommandBuffer commandBuffer, uint32_t frameIndex);
	void CreateDescriptor(VulkanContext& ctx);
};

#endif