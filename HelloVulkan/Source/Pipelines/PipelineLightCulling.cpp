#include "PipelineLightCulling.h"
#include "Configs.h"

#include <array>

PipelineLightCulling::PipelineLightCulling(
	VulkanContext& ctx,
	Lights* lights,
	ClusterForwardBuffers* cfBuffers) :
	PipelineBase(ctx,
		{
			.type_ = PipelineType::Compute
		}),
	lights_(lights),
	cfBuffers_(cfBuffers)
{
	CreateMultipleUniformBuffers(ctx, cfUBOBuffers_, sizeof(ClusterForwardUBO), AppConfig::FrameOverlapCount);
	CreateDescriptor(ctx);
	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_);

	std::string shaderFile = AppConfig::ShaderFolder + "ClusteredForward/LightCulling.comp";
	//std::string shaderFile = AppConfig::ShaderFolder + "ClusteredForward/LightCullingBatch.comp";

	CreateComputePipeline(ctx, shaderFile);
}

PipelineLightCulling::~PipelineLightCulling()
{
	for (auto uboBuffer : cfUBOBuffers_)
	{
		uboBuffer.Destroy();
	}
}

void PipelineLightCulling::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	const uint32_t frameIndex = ctx.GetFrameIndex();
	Execute(ctx, commandBuffer, frameIndex);
}

void PipelineLightCulling::Execute(VulkanContext& ctx, VkCommandBuffer commandBuffer, uint32_t frameIndex)
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
	
	// Batched version
	/*vkCmdDispatch(commandBuffer,
		1,
		1,
		6);*/

	VkBufferMemoryBarrier lightGridBarrier =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.srcQueueFamilyIndex = ctx.GetComputeFamily(),
		.dstQueueFamilyIndex = ctx.GetGraphicsFamily(),
		.buffer = cfBuffers_->lightCellsBuffers_[frameIndex].buffer_,
		.offset = 0,
		.size = cfBuffers_->lightCellsBuffers_[frameIndex].size_
	};

	const VkBufferMemoryBarrier lightIndicesBarrier =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.srcQueueFamilyIndex = ctx.GetComputeFamily(),
		.dstQueueFamilyIndex = ctx.GetGraphicsFamily(),
		.buffer = cfBuffers_->lightIndicesBuffers_[frameIndex].buffer_,
		.offset = 0,
		.size = cfBuffers_->lightIndicesBuffers_[frameIndex].size_,
	};

	const std::array<VkBufferMemoryBarrier, 2> barriers =
	{
		lightGridBarrier,
		lightIndicesBarrier
	};

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0,
		nullptr,
		static_cast<uint32_t>(barriers.size()),
		barriers.data(),
		0,
		nullptr);
}

void PipelineLightCulling::CreateDescriptor(VulkanContext& ctx)
{
	const uint32_t frameCount = static_cast<uint32_t>(AppConfig::FrameOverlapCount);
	constexpr VkShaderStageFlags stageFlag = VK_SHADER_STAGE_COMPUTE_BIT;

	DescriptorBuildInfo buildInfo;
	buildInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 0
	buildInfo.AddBuffer(lights_->GetVulkanBufferPtr(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 1
	buildInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 2
	buildInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 3
	buildInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 4
	buildInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stageFlag); // 5

	descriptor_.CreatePoolAndLayout(ctx, buildInfo, frameCount, 1u);

	for (size_t i = 0; i < frameCount; ++i)
	{
		buildInfo.UpdateBuffer(&(cfBuffers_->aabbBuffers_[i]), 0);
		buildInfo.UpdateBuffer(&(cfBuffers_->globalIndexCountBuffers_[i]), 2);
		buildInfo.UpdateBuffer(&(cfBuffers_->lightCellsBuffers_[i]), 3);
		buildInfo.UpdateBuffer(&(cfBuffers_->lightIndicesBuffers_[i]), 4);
		buildInfo.UpdateBuffer(&(cfUBOBuffers_[i]), 5);
		descriptor_.CreateSet(ctx, buildInfo.writes_, &(descriptorSets_[i]));
	}
}