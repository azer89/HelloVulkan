#ifndef PIPELINE_INFINITE_GRID
#define PIPELINE_INFINITE_GRID

#include "PipelineBase.h"

struct ResourcesShared;

class PipelineInfiniteGrid final : public PipelineBase
{
	PipelineInfiniteGrid(
		VulkanContext& ctx,
		ResourcesShared* resShared,
		uint8_t renderBit = 0u);
	~PipelineInfiniteGrid();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void ShouldRender(bool shouldRender) { shouldRender_ = shouldRender; };

private:
	bool shouldRender_;
};

#endif