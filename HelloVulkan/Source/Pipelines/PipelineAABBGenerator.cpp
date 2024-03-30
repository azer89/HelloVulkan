#include "PipelineAABBGenerator.h"
#include "ResourcesClusterForward.h"
#include "VulkanBarrier.h"
#include "Configs.h"

#include <iostream>

PipelineAABBGenerator::PipelineAABBGenerator(
	VulkanContext& ctx, 
	ResourcesClusterForward* resourcesCF) :
	PipelineBase(ctx,
	{
		.type_ = PipelineType::Compute
	}),
	resourcesCF_(resourcesCF)
{
	VulkanBuffer::CreateMultipleUniformBuffers(ctx, cfUBOBuffers_, sizeof(ClusterForwardUBO), AppConfig::FrameCount);
	CreateDescriptor(ctx);
	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_);
	CreateComputePipeline(ctx, AppConfig::ShaderFolder + "ClusteredForward/AABBGenerator.comp");
}

PipelineAABBGenerator::~PipelineAABBGenerator()
{
	for (auto uboBuffer : cfUBOBuffers_)
	{
		uboBuffer.Destroy();
	}
}

void PipelineAABBGenerator::OnWindowResized(VulkanContext& ctx)
{
	resourcesCF_->aabbDirty_ = true;
}

void PipelineAABBGenerator::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	uint32_t frameIndex = ctx.GetFrameIndex();
	if (!resourcesCF_->aabbDirty_)
	{
		return;
	}

	Execute(ctx, commandBuffer, frameIndex);

	resourcesCF_->aabbDirty_ = false;
}

void PipelineAABBGenerator::Execute(VulkanContext& ctx, VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
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

	ctx.InsertDebugLabel(commandBuffer, "PipelineFrustumCulling", 0xff99ff99);

	vkCmdDispatch(commandBuffer,
		static_cast<uint32_t>(ClusterForwardConfig::SliceCountX), // groupCountX
		static_cast<uint32_t>(ClusterForwardConfig::SliceCountY), // groupCountY
		static_cast<uint32_t>(ClusterForwardConfig::SliceCountZ)); // groupCountZ

	const VkBufferMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
		.pNext = nullptr,
		.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
		.srcQueueFamilyIndex = ctx.GetComputeFamily(),
		.dstQueueFamilyIndex = ctx.GetGraphicsFamily(),
		.buffer = resourcesCF_->aabbBuffer_.buffer_,
		.offset = 0,
		.size = resourcesCF_->aabbBuffer_.size_ };
	VulkanBarrier::CreateBufferBarrier(commandBuffer, &barrier, 1u);
}

void PipelineAABBGenerator::CreateDescriptor(VulkanContext& ctx)
{
	constexpr uint32_t frameCount = AppConfig::FrameCount;

	VulkanDescriptorInfo dsInfo;
	dsInfo.AddBuffer(&(resourcesCF_->aabbBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // 0
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // 1

	// Pool and layout
	descriptor_.CreatePoolAndLayout(ctx, dsInfo, frameCount, 1u);

	// Sets
	for (size_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(cfUBOBuffers_[i]), 1);

		descriptor_.CreateSet(ctx, dsInfo, &descriptorSets_[i]);
	}
}