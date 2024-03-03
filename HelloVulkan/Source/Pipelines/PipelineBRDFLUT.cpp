#include "PushConstants.h"
#include "PipelineBRDFLUT.h"
#include "VulkanBarrier.h"
#include "Configs.h"

PipelineBRDFLUT::PipelineBRDFLUT(
	VulkanContext& ctx) :
	PipelineBase(ctx, 
	{
		.type_ = PipelineType::Compute
	})
{
	outBuffer_.CreateBuffer(ctx, 
		IBLConfig::LUTBufferSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_AUTO,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

	// Push constants
	const std::vector<VkPushConstantRange> ranges =
	{{
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.offset = 0u,
		.size = sizeof(PushConstBRDFLUT)
	}};

	CreateDescriptor(ctx);
	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_, ranges);
	CreateComputePipeline(ctx, AppConfig::ShaderFolder + "BRDFLUT.comp");
}

PipelineBRDFLUT::~PipelineBRDFLUT()
{
	outBuffer_.Destroy();
}

void PipelineBRDFLUT::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
}

void PipelineBRDFLUT::CreateLUT(VulkanContext& ctx, VulkanImage* outputLUT)
{
	// Need to use std::vector because std::array will cause stack overflow, 
	// probably because the length is too long.
	std::vector<float> lutData(IBLConfig::LUTBufferSize, 0);

	Execute(ctx);

	// Copy the buffer content to an image
	// TODO Find a way so that compute shader can write to an image
	outBuffer_.DownloadBufferData(ctx, lutData.data(), IBLConfig::LUTBufferSize);
	outputLUT->CreateImageFromData(
		ctx,
		lutData.data(),
		IBLConfig::LUTWidth,
		IBLConfig::LUTHeight,
		1, // Mipmap count
		1, // Layer count
		VK_FORMAT_R32G32_SFLOAT);
	outputLUT->CreateImageView(
		ctx,
		VK_FORMAT_R32G32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT
	);

	outputLUT->CreateDefaultSampler(ctx);
}

void PipelineBRDFLUT::Execute(VulkanContext& ctx)
{
	VkCommandBuffer commandBuffer = ctx.BeginOneTimeComputeCommand();

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);

	PushConstBRDFLUT pc =
	{
		.width = IBLConfig::LUTWidth,
		.height = IBLConfig::LUTHeight,
		.sampleCount = IBLConfig::LUTSampleCount
	};
	vkCmdPushConstants(
		commandBuffer,
		pipelineLayout_,
		VK_SHADER_STAGE_COMPUTE_BIT,
		0,
		sizeof(PushConstBRDFLUT), &pc);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		pipelineLayout_,
		0, // firstSet
		1, // descriptorSetCount
		&descriptorSet_,
		0, // dynamicOffsetCount
		0); // pDynamicOffsets

	// Tell the GPU to do some compute
	vkCmdDispatch(commandBuffer, 
		static_cast<uint32_t>(IBLConfig::LUTWidth), // groupCountX
		static_cast<uint32_t>(IBLConfig::LUTHeight), // groupCountY
		1u); // groupCountZ
	
	// Set barrier
	VkMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.pNext = nullptr,
		.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT,
		.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT
	};
	VulkanBarrier::CreateMemoryBarrier(commandBuffer, &barrier, 1u);

	ctx.EndOneTimeComputeCommand(commandBuffer);
}

void PipelineBRDFLUT::CreateDescriptor(VulkanContext& ctx)
{
	VulkanDescriptorInfo dsInfo;
	dsInfo.AddBuffer(&outBuffer_, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
	descriptor_.CreatePoolAndLayout(ctx, dsInfo, 1u, 1u);
	descriptor_.CreateSet(ctx, dsInfo, &descriptorSet_);
}