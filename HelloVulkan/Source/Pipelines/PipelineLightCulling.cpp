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
		.size = VK_WHOLE_SIZE
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
		.size = VK_WHOLE_SIZE,
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
	const uint32_t imageCount = static_cast<uint32_t>(AppConfig::FrameOverlapCount);

	// Pool
	descriptor_.CreatePool(
		ctx,
		{
			.uboCount_ = 5u,
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
				.bindingCount_ = 5
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
		VkDescriptorBufferInfo bufferInfo2 = lights_->GetBufferInfo();
		VkDescriptorBufferInfo bufferInfo3 = cfBuffers_->globalIndexCountBuffers_[i].GetBufferInfo();
		VkDescriptorBufferInfo bufferInfo4 = cfBuffers_->lightCellsBuffers_[i].GetBufferInfo();
		VkDescriptorBufferInfo bufferInfo5 = cfBuffers_->lightIndicesBuffers_[i].GetBufferInfo();
		VkDescriptorBufferInfo bufferInfo6 = cfUBOBuffers_[i].GetBufferInfo();

		descriptor_.CreateSet(
			ctx,
			{
				{.bufferInfoPtr_ = &bufferInfo1, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
				{.bufferInfoPtr_ = &bufferInfo2, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
				{.bufferInfoPtr_ = &bufferInfo3, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
				{.bufferInfoPtr_ = &bufferInfo4, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
				{.bufferInfoPtr_ = &bufferInfo5, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
				{.bufferInfoPtr_ = &bufferInfo6, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }
			},
			&descriptorSets_[i]);
	}
}