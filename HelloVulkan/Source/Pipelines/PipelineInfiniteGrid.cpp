#include "PipelineInfiniteGrid.h"

#include "ResourcesShared.h"

PipelineInfiniteGrid::PipelineInfiniteGrid(
	VulkanContext& ctx,
	ResourcesShared* resShared,
	uint8_t renderBit = 0u) :
	PipelineBase(ctx,
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.msaaSamples_ = resShared->multiSampledColorImage_.multisampleCount_,
			//.topology_ = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,

			.vertexBufferBind_ = false,
			.depthTest_ = true,
			.depthWrite_ = false // Do not write to depth image
		}),
	shouldRender_(true) // TODO
{

}

PipelineInfiniteGrid::~PipelineInfiniteGrid()
{

}

void PipelineInfiniteGrid::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	if (!shouldRender_)
	{
		return;
	}

	const uint32_t frameIndex = ctx.GetFrameIndex();
}
