#ifndef PIPELINE_AABB_RENDER
#define PIPELINE_AABB_RENDER

#include "VulkanContext.h"
#include "PipelineBase.h"
#include "ResourcesShared.h"
#include "Configs.h"

class PipelineAABBRender final : public PipelineBase
{
public:
	PipelineAABBRender(
		VulkanContext& ctx,
		ResourcesShared* resShared,
		Scene* scene,
		uint8_t renderBit = 0u
	);
	~PipelineAABBRender();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void UpdateFromInputContext(VulkanContext& ctx, UIData& inputContext) override
	{
		shouldRender_ = inputContext.renderDebug_;
	}

private:
	void CreateDescriptor(VulkanContext& ctx);

private:
	Scene* scene_;
	std::array<VkDescriptorSet, AppConfig::FrameCount> descriptorSets_;
	bool shouldRender_;
};

#endif