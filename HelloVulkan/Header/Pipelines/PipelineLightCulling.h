#ifndef PIPELINE_LIGHT_CULLING
#define PIPELINE_LIGHT_CULLING

#include "PipelineBase.h"
#include "VulkanContext.h"
#include "Configs.h"

#include <array>

struct ResourcesClusterForward;
struct ResourcesLight;

/*
Clustered Forward
*/
class PipelineLightCulling final : public PipelineBase
{
public:
	PipelineLightCulling(VulkanContext& ctx, ResourcesLight* resLight, ResourcesClusterForward* resCF);
	~PipelineLightCulling();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void ResetGlobalIndex(VulkanContext& ctx);

	void SetClusterForwardUBO(VulkanContext& ctx, ClusterForwardUBO ubo);

private:
	ResourcesLight* resLight_;
	ResourcesClusterForward* resCF_;

	std::vector<VulkanBuffer> cfUBOBuffers_;
	std::array<VkDescriptorSet, AppConfig::FrameOverlapCount> descriptorSets_;

private:
	void Execute(VulkanContext& ctx, VkCommandBuffer commandBuffer, uint32_t frameIndex);
	void CreateDescriptor(VulkanContext& ctx);
};

#endif