#include "PipelineAABBGenerator.h"
#include "Configs.h"

#include <iostream>

PipelineAABBGenerator::PipelineAABBGenerator(
	VulkanDevice& vkDev, 
	ClusterForwardBuffers* cfBuffers) :
	PipelineBase(vkDev,
	{
		.type_ = PipelineType::Compute
	}),
	cfBuffers_(cfBuffers)
{
	CreateMultipleUniformBuffers(vkDev, cfUBOBuffers_, sizeof(ClusterForwardUBO), AppConfig::FrameOverlapCount);
	CreateDescriptor(vkDev);
	CreatePipelineLayout(vkDev, descriptor_.layout_, &pipelineLayout_);
	CreateComputePipeline(vkDev, AppConfig::ShaderFolder + "ClusteredForward/AABBGenerator.comp");
}

PipelineAABBGenerator::~PipelineAABBGenerator()
{
	for (auto uboBuffer : cfUBOBuffers_)
	{
		uboBuffer.Destroy();
	}
}

void PipelineAABBGenerator::OnWindowResized(VulkanDevice& vkDev)
{
	cfBuffers_->SetAABBDirty();
}

void PipelineAABBGenerator::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer)
{
	uint32_t frameIndex = vkDev.GetFrameIndex();
	if (!cfBuffers_->IsAABBDirty(frameIndex))
	{
		return;
	}

	Execute(vkDev, commandBuffer, frameIndex);

	cfBuffers_->SetAABBClean(frameIndex);
}

void PipelineAABBGenerator::Execute(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, uint32_t frameIndex)
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
		.srcQueueFamilyIndex = vkDev.GetComputeFamily(),
		.dstQueueFamilyIndex = vkDev.GetGraphicsFamily(),
		.buffer = cfBuffers_->aabbBuffers_[frameIndex].buffer_,
		.offset = 0,
		.size = VK_WHOLE_SIZE };
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

void PipelineAABBGenerator::CreateDescriptor(VulkanDevice& vkDev)
{
	uint32_t imageCount = AppConfig::FrameOverlapCount;

	// Pool
	descriptor_.CreatePool(
		vkDev,
		{
			.uboCount_ = 1u,
			.ssboCount_ = 1u,
			.samplerCount_ = 0u,
			.frameCount_ = imageCount,
			.setCountPerFrame_ = 1u
		});

	// Layout
	descriptor_.CreateLayout(vkDev,
	{
		{
			.descriptorType_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_COMPUTE_BIT,
			.bindingCount_ = 1
		},
		{
			.descriptorType_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_COMPUTE_BIT,
			.bindingCount_ = 1
		}
	});

	// Set
	descriptorSets_.resize(imageCount);
	for (size_t i = 0; i < imageCount; ++i)
	{
		VkDescriptorBufferInfo bufferInfo1 = { cfBuffers_->aabbBuffers_[i].buffer_, 0, VK_WHOLE_SIZE};
		VkDescriptorBufferInfo bufferInfo2 = { cfUBOBuffers_[i].buffer_, 0, VK_WHOLE_SIZE};

		descriptor_.CreateSet(
		vkDev,
		{
			{.bufferInfoPtr_ = &bufferInfo1, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
			{.bufferInfoPtr_ = &bufferInfo2, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }
		},
		&descriptorSets_[i]);
	}
}