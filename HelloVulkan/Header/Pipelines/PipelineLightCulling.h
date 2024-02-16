#ifndef PIPELINE_LIGHT_CULLING
#define PIPELINE_LIGHT_CULLING

#include "PipelineBase.h"
#include "VulkanContext.h"
#include "ClusterForwardBuffers.h"
#include "Light.h"
#include "Configs.h"

#include <array>

/*
Clustered Forward
*/
class PipelineLightCulling final : public PipelineBase
{
public:
	PipelineLightCulling(VulkanContext& ctx, Lights* lights, ClusterForwardBuffers* cfBuffers);
	~PipelineLightCulling();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void ResetGlobalIndex(VulkanContext& ctx)
	{
		uint32_t zeroValue = 0u;
		uint32_t frameIndex = ctx.GetFrameIndex();
		cfBuffers_->globalIndexCountBuffers_[frameIndex].UploadBufferData(ctx, 0, &zeroValue, sizeof(uint32_t));
	}

	void SetClusterForwardUBO(VulkanContext& ctx, ClusterForwardUBO ubo)
	{
		size_t frameIndex = ctx.GetFrameIndex();
		cfUBOBuffers_[frameIndex].UploadBufferData(ctx, 0, &ubo, sizeof(ClusterForwardUBO));
	}

private:
	Lights* lights_;
	ClusterForwardBuffers* cfBuffers_;

	std::vector<VulkanBuffer> cfUBOBuffers_;
	std::array<VkDescriptorSet, AppConfig::FrameOverlapCount> descriptorSets_;

private:
	void Execute(VulkanContext& ctx, VkCommandBuffer commandBuffer, uint32_t frameIndex);
	void CreateDescriptor(VulkanContext& ctx);
};

#endif