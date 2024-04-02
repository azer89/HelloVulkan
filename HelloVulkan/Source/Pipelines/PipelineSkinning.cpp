#include "PipelineSkinning.h"
#include "VulkanBarrier.h"

PipelineSkinning::PipelineSkinning(VulkanContext& ctx, Scene* scene) :
	PipelineBase(ctx,
	{
		.type_ = PipelineType::Compute
	}),
	scene_(scene)
{
	CreateDescriptor(ctx);
	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_);
	CreateComputePipeline(ctx, AppConfig::ShaderFolder + "Skinning.comp");
}

PipelineSkinning::~PipelineSkinning()
{
}

void PipelineSkinning::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	const uint32_t frameIndex = ctx.GetFrameIndex();
	Execute(ctx, commandBuffer, frameIndex);
}

void PipelineSkinning::Execute(VulkanContext& ctx, VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "Skinning", tracy::Color::Lime);

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

	ctx.InsertDebugLabel(commandBuffer, "PipelineSkinning", 0xffff9999);

	constexpr float workgroupSize = 256.f;
	const float vertexSize = static_cast<float>(scene_->sceneData_.vertices_.size());
	const uint32_t groupSizeX = static_cast<uint32_t>(std::ceil(vertexSize / workgroupSize));
	vkCmdDispatch(commandBuffer, groupSizeX, 1, 1);

	const VkBufferMemoryBarrier2 bufferBarrier =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
		.pNext = nullptr,
		.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, // Skinned vertex buffer is read in vertex shader
		.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT,
		.srcQueueFamilyIndex = ctx.GetComputeFamily(),
		.dstQueueFamilyIndex = ctx.GetGraphicsFamily(),
		.buffer = scene_->vertexBuffer_.buffer_,
		.offset = 0,
		.size = scene_->vertexBuffer_.size_,
	};
	VulkanBarrier::CreateBufferBarrier(commandBuffer, &bufferBarrier, 1u);
}

void PipelineSkinning::CreateDescriptor(VulkanContext& ctx)
{
	constexpr uint32_t frameCount = AppConfig::FrameCount;
	constexpr VkShaderStageFlags stageFlag = VK_SHADER_STAGE_COMPUTE_BIT;
	VulkanDescriptorInfo dsInfo;

	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 0
	dsInfo.AddBuffer(&(scene_->boneIDBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 1
	dsInfo.AddBuffer(&(scene_->boneWeightBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 2
	dsInfo.AddBuffer(&(scene_->skinningIndicesBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 3
	dsInfo.AddBuffer(&(scene_->preSkinningVertexBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 4 Input
	dsInfo.AddBuffer(&(scene_->vertexBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, stageFlag); // 5 Output

	descriptor_.CreatePoolAndLayout(ctx, dsInfo, frameCount, 1u);

	for (size_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(scene_->boneMatricesBuffers_[i]), 0);
		descriptor_.CreateSet(ctx, dsInfo, &(descriptorSets_[i]));
	}
}