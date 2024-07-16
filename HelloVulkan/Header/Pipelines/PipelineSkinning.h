#ifndef PIPELINE_SKINNING
#define PIPELINE_SKINNING

#include "VulkanContext.h"
#include "PipelineBase.h"
#include "Scene.h"

class PipelineSkinning final : public PipelineBase
{
public:
	PipelineSkinning(VulkanContext& ctx, Scene* scene);
	~PipelineSkinning();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;
	void SetCameraUBO(VulkanContext& ctx, CameraUBO& ubo) override {}

private:
	void Execute(VulkanContext& ctx, VkCommandBuffer commandBuffer, uint32_t frameIndex);
	void CreateDescriptor(VulkanContext& ctx);

private:
	Scene* scene_{};
	std::array<VkDescriptorSet, AppConfig::FrameCount> descriptorSets_{};
};

#endif