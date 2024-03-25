#ifndef PIPELINE_INFINITE_GRID
#define PIPELINE_INFINITE_GRID

#include "PipelineBase.h"
#include "ResourcesShared.h"

class PipelineInfiniteGrid final : public PipelineBase
{
public:
	PipelineInfiniteGrid(
		VulkanContext& ctx,
		ResourcesShared* resourcesShared,
		float yPosition,
		uint8_t renderBit = 0);
	~PipelineInfiniteGrid();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;
	void ShouldRender(bool shouldRender) { shouldRender_ = shouldRender; }

	void UpdateFromInputContext(VulkanContext& ctx, InputContext& inputContext) override
	{
		shouldRender_ = inputContext.renderInfiniteGrid_;
	}

private:
	void CreateDescriptor(VulkanContext& ctx);

private:
	float yPosition_;
	bool shouldRender_;
	std::array<VkDescriptorSet, AppConfig::FrameCount> descriptorSets_;
};

#endif