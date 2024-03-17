#ifndef PIPELINE_FRUSTUM_CULLING
#define PIPELINE_FRUSTUM_CULLING

#include "PipelineBase.h"
#include "VulkanContext.h"
#include "Configs.h"

class Scene;

class PipelineFrustumCulling final : public PipelineBase
{
public:
	PipelineFrustumCulling(VulkanContext& ctx, Scene* scene);
	~PipelineFrustumCulling();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

private:
	void Execute(VulkanContext& ctx, VkCommandBuffer commandBuffer, uint32_t frameIndex);

	void CreateDescriptor(VulkanContext& ctx);

private:
	Scene* scene_;
	std::array<VkDescriptorSet, AppConfig::FrameCount> descriptorSets_;
};

#endif