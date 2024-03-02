#include "PipelineAABBGenerator.h"
#include "Configs.h"

#include <iostream>

PipelineAABBGenerator::PipelineAABBGenerator(
	VulkanContext& ctx, 
	ClusterForwardBuffers* cfBuffers) :
	PipelineBase(ctx,
	{
		.type_ = PipelineType::Compute
	}),
	cfBuffers_(cfBuffers)
{
	CreateMultipleUniformBuffers(ctx, cfUBOBuffers_, sizeof(ClusterForwardUBO), AppConfig::FrameOverlapCount);
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
	cfBuffers_->SetAABBDirty();
}

void PipelineAABBGenerator::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	uint32_t frameIndex = ctx.GetFrameIndex();
	if (!cfBuffers_->IsAABBDirty(frameIndex))
	{
		return;
	}

	Execute(ctx, commandBuffer, frameIndex);

	cfBuffers_->SetAABBClean(frameIndex);
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

	vkCmdDispatch(commandBuffer,
		static_cast<uint32_t>(ClusterForwardConfig::sliceCountX), // groupCountX
		static_cast<uint32_t>(ClusterForwardConfig::sliceCountY), // groupCountY
		static_cast<uint32_t>(ClusterForwardConfig::sliceCountZ)); // groupCountZ

	VkBufferMemoryBarrier2 barrierInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
		.pNext = nullptr,
		.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.srcQueueFamilyIndex = ctx.GetComputeFamily(),
		.dstQueueFamilyIndex = ctx.GetGraphicsFamily(),
		.buffer = cfBuffers_->aabbBuffers_[frameIndex].buffer_,
		.offset = 0,
		.size = cfBuffers_->aabbBuffers_[frameIndex].size_ };
	VkDependencyInfo dependencyInfo = {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.bufferMemoryBarrierCount = 1u,
		.pBufferMemoryBarriers = &barrierInfo
	};
	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}

void PipelineAABBGenerator::CreateDescriptor(VulkanContext& ctx)
{
	uint32_t frameCount = AppConfig::FrameOverlapCount;

	DescriptorBuildInfo buildInfo;
	buildInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // 0
	buildInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT); // 1

	// Pool and layout
	descriptor_.CreatePoolAndLayout(ctx, buildInfo, frameCount, 1u);

	// Sets
	for (size_t i = 0; i < frameCount; ++i)
	{
		buildInfo.UpdateBuffer(&(cfBuffers_->aabbBuffers_[i]), 0);
		buildInfo.UpdateBuffer(&(cfUBOBuffers_[i]), 1);

		descriptor_.CreateSet(ctx, buildInfo, &descriptorSets_[i]);
	}
}