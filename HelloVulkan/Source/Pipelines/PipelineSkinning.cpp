#include "PipelineSkinning.h"


PipelineSkinning::PipelineSkinning(VulkanContext& ctx, Scene* scene) :
	PipelineBase(ctx,
	{
		.type_ = PipelineType::Compute
	})
{
}

PipelineSkinning::~PipelineSkinning()
{
}

void PipelineSkinning::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	const uint32_t frameIndex = ctx.GetFrameIndex();
	Execute(ctx, commandBuffer, frameIndex);
}

void PipelineSkinning::Execute(VulkanContext& ctx, VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "Skinning", tracy::Color::Lime);

	/*const VkBufferMemoryBarrier2 bufferBarrier =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
		.pNext = nullptr,
		.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.srcQueueFamilyIndex = ctx.GetComputeFamily(),
		.dstQueueFamilyIndex = ctx.GetGraphicsFamily(),
		.buffer = scene_->indirectBuffer_.buffer_,
		.offset = 0,
		.size = scene_->indirectBuffer_.size_,
	};
	VulkanBarrier::CreateBufferBarrier(commandBuffer, &bufferBarrier, 1u);*/
}

void PipelineSkinning::CreateDescriptor(VulkanContext& ctx)
{
	constexpr uint32_t frameCount = AppConfig::FrameCount;
	constexpr VkShaderStageFlags stageFlag = VK_SHADER_STAGE_COMPUTE_BIT;
	VulkanDescriptorInfo dsInfo;

	// TODO

	for (size_t i = 0; i < frameCount; ++i)
	{
		// TODO
	}
}