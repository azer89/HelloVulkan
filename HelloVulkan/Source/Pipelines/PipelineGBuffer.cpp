#include "PipelineGBuffer.h"

PipelineGBuffer::PipelineGBuffer(VulkanContext& ctx,
	Scene* scene,
	ResourcesGBuffer* resourcesGBuffer,
	uint8_t renderBit) :
	PipelineBase(ctx,
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.vertexBufferBind_ = false,
		}),
		scene_(scene),
		resourcesGBuffer_(resourcesGBuffer)
{
	VulkanBuffer::CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(CameraUBO), AppConfig::FrameCount);
	CreateBDABuffer(ctx); // Buffer device address
	CreateDescriptor(ctx);
	renderPass_.CreateOffScreenGBuffer(
		ctx,
		{
			resourcesGBuffer_->position_.imageFormat_,
			resourcesGBuffer_->normal_.imageFormat_
		},
		renderBit | RenderPassBit::ColorClear | RenderPassBit::DepthClear);
	framebuffer_.CreateResizeable(
		ctx,
		renderPass_.GetHandle(),
		{
			&(resourcesGBuffer_->position_),
			&(resourcesGBuffer_->normal_),
			&(resourcesGBuffer_->depth_)
		},
		IsOffscreen());
	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_);
	AddOverridingColorBlendAttachment(0xf, VK_FALSE); // resourcesGBuffer_->position_
	AddOverridingColorBlendAttachment(0xf, VK_FALSE); // resourcesGBuffer_->normal_
	CreateGraphicsPipeline(
		ctx,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "GBuffer/GBuffer.vert",
			AppConfig::ShaderFolder + "GBuffer/GBuffer.frag"
		},
		&pipeline_
	);
}

PipelineGBuffer::~PipelineGBuffer()
{
	bdaBuffer_.Destroy();
}

void PipelineGBuffer::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "G_Buffer", tracy::Color::Orange4);

	const uint32_t frameIndex = ctx.GetFrameIndex();
	renderPass_.BeginRenderPass(ctx, commandBuffer, framebuffer_.GetFramebuffer());

	BindPipeline(ctx, commandBuffer);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout_,
		0u, // firstSet
		1u, // descriptorSetCount
		&(descriptorSets_[frameIndex]),
		0u, // dynamicOffsetCount
		nullptr); // pDynamicOffsets

	ctx.InsertDebugLabel(commandBuffer, "PipelineGBuffer", 0xff99ff99);

	vkCmdDrawIndirect(
		commandBuffer,
		scene_->indirectBuffer_.buffer_,
		0, // offset
		scene_->GetInstanceCount(),
		sizeof(VkDrawIndirectCommand));

	vkCmdEndRenderPass(commandBuffer);

	VulkanImage::TransitionLayoutCommand(
		commandBuffer,
		resourcesGBuffer_->position_.image_,
		resourcesGBuffer_->position_.imageFormat_,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VulkanImage::TransitionLayoutCommand(
		commandBuffer,
		resourcesGBuffer_->normal_.image_,
		resourcesGBuffer_->normal_.imageFormat_,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void PipelineGBuffer::CreateBDABuffer(VulkanContext& ctx)
{
	const BDA bda = scene_->GetBDA();
	const VkDeviceSize bdaSize = sizeof(BDA);
	bdaBuffer_.CreateBuffer(
		ctx,
		bdaSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	bdaBuffer_.UploadBufferData(ctx, &bda, bdaSize);
}

void PipelineGBuffer::CreateDescriptor(VulkanContext& ctx)
{
	constexpr uint32_t frameCount = AppConfig::FrameCount;

	VulkanDescriptorInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 0
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 1
	dsInfo.AddBuffer(&bdaBuffer_, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 2
	dsInfo.AddImageArray(scene_->GetImageInfos());

	// Pool and layout
	descriptor_.CreatePoolAndLayout(ctx, dsInfo, frameCount, 1u);

	// Sets
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(cameraUBOBuffers_[i]), 0);
		dsInfo.UpdateBuffer(&(scene_->modelSSBOBuffers_[i]), 1);
		descriptor_.CreateSet(ctx, dsInfo, &(descriptorSets_[i]));
	}
}