#include "PipelineLightCulling.h"
#include "ResourcesClusterForward.h"
#include "ResourcesLight.h"
#include "VulkanBarrier.h"
#include "Configs.h"

#include <array>

PipelineLightCulling::PipelineLightCulling(
	VulkanContext& ctx,
	ResourcesLight* resourcesLight,
	ResourcesClusterForward* resourcesCF) :
	PipelineBase(ctx,
		{
			.type_ = PipelineType::Compute
		}),
	resourcesLight_(resourcesLight),
	resourcesCF_(resourcesCF)
{
	VulkanBuffer::CreateMultipleUniformBuffers(ctx, cfUBOBuffers_, sizeof(ClusterForwardUBO), AppConfig::FrameCount);
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

void PipelineLightCulling::ResetGlobalIndex(VulkanContext& ctx)
{
	const uint32_t frameIndex = ctx.GetFrameIndex();
	constexpr uint32_t zeroValue = 0u;
	resourcesCF_->globalIndexCountBuffers_[frameIndex].UploadBufferData(ctx, &zeroValue, sizeof(uint32_t));
}

void PipelineLightCulling::SetClusterForwardUBO(VulkanContext& ctx, ClusterForwardUBO& ubo)
{
	const size_t frameIndex = ctx.GetFrameIndex();
	cfUBOBuffers_[frameIndex].UploadBufferData(ctx, &ubo, sizeof(ClusterForwardUBO));
}

void PipelineLightCulling::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "Light_Culling", tracy::Color::LawnGreen);
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

	ctx.InsertDebugLabel(commandBuffer, "PipelineLightCulling", 0xff9999ff);

	vkCmdDispatch(commandBuffer,
		static_cast<uint32_t>(ClusterForwardConfig::SliceCountX), // groupCountX
		static_cast<uint32_t>(ClusterForwardConfig::SliceCountY), // groupCountY
		static_cast<uint32_t>(ClusterForwardConfig::SliceCountZ)); // groupCountZ
	
	// Batched version
	/*vkCmdDispatch(commandBuffer,
		1,
		1,
		6);*/

	const VkBufferMemoryBarrier2 lightGridBarrier =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
		.pNext = nullptr,
		.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
		.srcQueueFamilyIndex = ctx.GetComputeFamily(),
		.dstQueueFamilyIndex = ctx.GetGraphicsFamily(),
		.buffer = resourcesCF_->lightCellsBuffer_.buffer_,
		.offset = 0,
		.size = resourcesCF_->lightCellsBuffer_.size_
	};
	const VkBufferMemoryBarrier2 lightIndicesBarrier =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
		.pNext = nullptr,
		.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
		.srcQueueFamilyIndex = ctx.GetComputeFamily(),
		.dstQueueFamilyIndex = ctx.GetGraphicsFamily(),
		.buffer = resourcesCF_->lightIndicesBuffer_.buffer_,
		.offset = 0,
		.size = resourcesCF_->lightIndicesBuffer_.size_,
	};
	const std::array<VkBufferMemoryBarrier2, 2> barriers =
	{
		lightGridBarrier,
		lightIndicesBarrier
	};
	VulkanBarrier::CreateBufferBarrier(commandBuffer, barriers.data(), static_cast<uint32_t>(barriers.size()));
}

void PipelineLightCulling::CreateDescriptor(VulkanContext& ctx)
{
	const uint32_t frameCount = static_cast<uint32_t>(AppConfig::FrameCount);
	constexpr VkShaderStageFlags stageFlag = VK_SHADER_STAGE_COMPUTE_BIT;

	VulkanDescriptorInfo dsInfo;
	dsInfo.AddBuffer(&(resourcesCF_->aabbBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 0
	dsInfo.AddBuffer(resourcesLight_->GetVulkanBufferPtr(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 1
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 2
	dsInfo.AddBuffer(&(resourcesCF_->lightCellsBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 3
	dsInfo.AddBuffer(&(resourcesCF_->lightIndicesBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 4
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stageFlag); // 5

	descriptor_.CreatePoolAndLayout(ctx, dsInfo, frameCount, 1u);

	for (size_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(resourcesCF_->globalIndexCountBuffers_[i]), 2);
		dsInfo.UpdateBuffer(&(cfUBOBuffers_[i]), 5);
		descriptor_.CreateSet(ctx, dsInfo, &(descriptorSets_[i]));
	}
}