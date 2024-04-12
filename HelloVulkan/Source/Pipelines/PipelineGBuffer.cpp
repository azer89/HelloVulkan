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
	renderPass_.CreateOffScreenMultipleRenderTargets(
		ctx,
		{ resourcesGBuffer_->position_.imageFormat_, resourcesGBuffer_->normal_.imageFormat_ },
		renderBit);
	framebuffer_.CreateResizeable(
		ctx,
		renderPass_.GetHandle(),
		{
			&(resourcesGBuffer_->position_),
			&(resourcesGBuffer_->normal_)
		},
		IsOffscreen());
}

PipelineGBuffer::~PipelineGBuffer()
{
	bdaBuffer_.Destroy();
}

void PipelineGBuffer::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "G_Buffer", tracy::Color::Orange4);
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

	// Pool and layout
	descriptor_.CreatePoolAndLayout(ctx, dsInfo, frameCount, 1u);

	// Sets
	descriptorSets_.resize(frameCount);
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(cameraUBOBuffers_[i]), 0);
		dsInfo.UpdateBuffer(&(scene_->modelSSBOBuffers_[i]), 1);
		descriptor_.CreateSet(ctx, dsInfo, &(descriptorSets_[i]));
	}
}