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

	VkBufferMemoryBarrier barrierInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.srcQueueFamilyIndex = ctx.GetComputeFamily(),
		.dstQueueFamilyIndex = ctx.GetGraphicsFamily(),
		.buffer = cfBuffers_->aabbBuffers_[frameIndex].buffer_,
		.offset = 0,
		.size = cfBuffers_->aabbBuffers_[frameIndex].size_ };
	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, //srcStageMask
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // dstStageMask
		0,
		0,
		nullptr,
		1,
		&barrierInfo,
		0, nullptr);
}

void PipelineAABBGenerator::CreateDescriptor(VulkanContext& ctx)
{
	uint32_t imageCount = AppConfig::FrameOverlapCount;

	// Pool
	descriptor_.CreatePool(
		ctx,
		{
			.uboCount_ = 1u,
			.ssboCount_ = 1u,
			.samplerCount_ = 0u,
			.frameCount_ = imageCount,
			.setCountPerFrame_ = 1u
		});

	// Layout
	descriptor_.CreateLayout(ctx,
	{
		{
			.type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_COMPUTE_BIT,
			.bindingCount_ = 1
		},
		{
			.type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_COMPUTE_BIT,
			.bindingCount_ = 1
		}
	});

	// Set
	for (size_t i = 0; i < imageCount; ++i)
	{
		VkDescriptorBufferInfo bufferInfo1 = cfBuffers_->aabbBuffers_[i].GetBufferInfo();
		VkDescriptorBufferInfo bufferInfo2 = cfUBOBuffers_[i].GetBufferInfo();

		descriptor_.CreateSet(
		ctx,
		{
			{.bufferInfoPtr_ = &bufferInfo1, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
			{.bufferInfoPtr_ = &bufferInfo2, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }
		},
		&descriptorSets_[i]);
	}
}