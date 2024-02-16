#include "PushConstants.h"
#include "PipelineBRDFLUT.h"
#include "VulkanShader.h"
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
	std::vector<VkPushConstantRange> ranges =
	{{
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.offset = 0u,
		.size = sizeof(PushConstantsBRDFLUT)
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
	std::vector<float> lutData(IBLConfig::LUTBufferSize, 0);

	Execute(ctx);

	// Copy the buffer content to an image
	// TODO Find a way so that compute shader can write to an image
	outBuffer_.DownloadBufferData(ctx, 0, lutData.data(), IBLConfig::LUTBufferSize);
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

	PushConstantsBRDFLUT pc =
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
		sizeof(PushConstantsBRDFLUT), &pc);

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

	// https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples-(Legacy-synchronization-APIs)#cpu-read-back-of-data-written-by-a-compute-shader
	VkMemoryBarrier readoutBarrier = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT, // Write access to SSBO
		.dstAccessMask = VK_ACCESS_HOST_READ_BIT // Read SSBO by a host / CPU
	};

	vkCmdPipelineBarrier(
		commandBuffer, // commandBuffer
		// Compute shader
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // srcStageMask
		// Host / CPU
		VK_PIPELINE_STAGE_HOST_BIT, // dstStageMask
		0, // dependencyFlags
		1, // memoryBarrierCount
		&readoutBarrier, // pMemoryBarriers
		0, // bufferMemoryBarrierCount 
		nullptr, // pBufferMemoryBarriers
		0, // imageMemoryBarrierCount
		nullptr); // pImageMemoryBarriers

	ctx.EndOneTimeComputeCommand(commandBuffer);
}

void PipelineBRDFLUT::CreateDescriptor(VulkanContext& ctx)
{
	// Pool
	descriptor_.CreatePool(
		ctx,
		{
			.uboCount_ = 0u,
			.ssboCount_ = 1u,
			.samplerCount_ = 0u,
			.frameCount_ = 1u,
			.setCountPerFrame_ = 1u
		});

	// Layout
	descriptor_.CreateLayout(ctx,
	{
		{
			.descriptorType_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_COMPUTE_BIT,
			.bindingCount_ = 1
		}
	});

	// Set
	VkDescriptorBufferInfo outBufferInfo = { outBuffer_.buffer_, 0, VK_WHOLE_SIZE };

	descriptor_.CreateSet(
		ctx,
		{
			{.bufferInfoPtr_ = &outBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }
		},
		&descriptorSet_);
}