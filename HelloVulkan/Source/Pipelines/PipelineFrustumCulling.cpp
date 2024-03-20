#include "PipelineFrustumCulling.h"
#include "VulkanBarrier.h"
#include "Scene.h"

PipelineFrustumCulling::PipelineFrustumCulling(VulkanContext& ctx, Scene* scene) :
	PipelineBase(ctx,
	{
		.type_ = PipelineType::Compute
	}),
	scene_(scene)
{
	VulkanBuffer::CreateMultipleUniformBuffers(ctx, frustumBuffers_, sizeof(FrustumUBO), AppConfig::FrameCount);
	CreateDescriptor(ctx);
	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_);
	std::string shaderFile = AppConfig::ShaderFolder + "FrustumCulling.comp";
	CreateComputePipeline(ctx, shaderFile);
}

PipelineFrustumCulling::~PipelineFrustumCulling()
{
	for (auto buffer : frustumBuffers_)
	{
		buffer.Destroy();
	}
}

void PipelineFrustumCulling::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	const uint32_t frameIndex = ctx.GetFrameIndex();
	Execute(ctx, commandBuffer, frameIndex);
}

void PipelineFrustumCulling::Execute(VulkanContext& ctx, VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "Frustum_Culling", tracy::Color::ForestGreen);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		pipelineLayout_,
		0, // firstSet
		1, // descriptorSetCount
		&descriptorSets_[frameIndex],
		0, // dynamicOffsetCount
		0); // pDynamicOffsets

	vkCmdDispatch(commandBuffer, scene_->GetInstanceCount(), 1, 1);

	const VkBufferMemoryBarrier2 bufferBarrier =
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
	VulkanBarrier::CreateBufferBarrier(commandBuffer, &bufferBarrier, 1u);
}

void PipelineFrustumCulling::CreateDescriptor(VulkanContext& ctx)
{
	const uint32_t frameCount = static_cast<uint32_t>(AppConfig::FrameCount);
	constexpr VkShaderStageFlags stageFlag = VK_SHADER_STAGE_COMPUTE_BIT;

	VulkanDescriptorInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stageFlag); // 0
	dsInfo.AddBuffer(&(scene_->transformedBoundingBoxBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 1
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 2
	descriptor_.CreatePoolAndLayout(ctx, dsInfo, frameCount, 1u);
	for (size_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(frustumBuffers_[i]), 0);
		dsInfo.UpdateBuffer(&(scene_->indirectBuffer_), 2);
		descriptor_.CreateSet(ctx, dsInfo, &(descriptorSets_[i]));
	}
}