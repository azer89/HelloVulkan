#ifndef PIPELINE_AABB_GENERATOR
#define PIPELINE_AABB_GENERATOR

#include "PipelineBase.h"
#include "VulkanContext.h"
#include "ResourcesClusterForward.h"
#include "Configs.h"

#include <array>

/*
Generate voxelized frustum for clustered forward
*/
class PipelineAABBGenerator final : public PipelineBase
{
public:
	PipelineAABBGenerator(VulkanContext& ctx, ResourcesClusterForward* resourcesCF);
	~PipelineAABBGenerator();

	void SetCameraUBO(VulkanContext& ctx, CameraUBO& ubo) override {}
	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;
	void OnWindowResized(VulkanContext& ctx) override;

	void SetClusterForwardUBO(VulkanContext& ctx, ClusterForwardUBO& ubo)
	{
		const size_t frameIndex = ctx.GetFrameIndex();
		cfUBOBuffers_[frameIndex].UploadBufferData(ctx, &ubo, sizeof(ClusterForwardUBO));
	}

private:
	ResourcesClusterForward* resourcesCF_;

	std::vector<VulkanBuffer> cfUBOBuffers_;
	std::array<VkDescriptorSet, AppConfig::FrameCount> descriptorSets_;

private:
	void Execute(VulkanContext& ctx, VkCommandBuffer commandBuffer, uint32_t frameIndex);
	void CreateDescriptor(VulkanContext& ctx);
};

#endif