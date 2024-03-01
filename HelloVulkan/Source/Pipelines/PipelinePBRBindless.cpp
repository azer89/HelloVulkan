#include "PipelinePBRBindless.h"
#include "VulkanUtility.h"
#include "Configs.h"

#include <vector>
#include <array>

// Constants
constexpr uint32_t UBO_COUNT = 2;
constexpr uint32_t SSBO_COUNT = 4;
constexpr uint32_t PBR_ENV_TEXTURE_COUNT = 3; // Specular, diffuse, and BRDF LUT

PipelinePBRBindless::PipelinePBRBindless(
	VulkanContext& ctx,
	Scene* scene,
	Lights* lights,
	IBLResources* iblResources,
	VulkanImage* depthImage,
	VulkanImage* offscreenColorImage,
	uint8_t renderBit) :
	PipelineBase(ctx, 
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.msaaSamples_ = offscreenColorImage->multisampleCount_,
			.vertexBufferBind_ = false,
		}
	),
	scene_(scene),
	lights_(lights),
	iblResources_(iblResources)
{
	// Per frame UBO
	CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(CameraUBO), AppConfig::FrameOverlapCount);

	CreateIndirectBuffers(ctx, scene_, indirectBuffers_);

	CreateDescriptor(ctx);

	// Note that this pipeline is offscreen rendering
	renderPass_.CreateOffScreenRenderPass(ctx, renderBit, config_.msaaSamples_);

	framebuffer_.CreateResizeable(
		ctx, 
		renderPass_.GetHandle(), 
		{
			offscreenColorImage,
			depthImage
		}, 
		IsOffscreen());

	// Push constants and pipeline layout
	const std::vector<VkPushConstantRange> ranges =
	{{
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset = 0u,
		.size = sizeof(PushConstantPBR),
	}};
	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_, ranges);

	CreateGraphicsPipeline(
		ctx,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "Bindless//Scene.vert",
			AppConfig::ShaderFolder + "Bindless//Scene.frag"
		},
		&pipeline_
	);
}

PipelinePBRBindless::~PipelinePBRBindless()
{
	for (auto& buffer : indirectBuffers_)
	{
		buffer.Destroy();
	}
}

void PipelinePBRBindless::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	const uint32_t frameIndex = ctx.GetFrameIndex();
	renderPass_.BeginRenderPass(ctx, commandBuffer, framebuffer_.GetFramebuffer());

	BindPipeline(ctx, commandBuffer);

	vkCmdPushConstants(
		commandBuffer,
		pipelineLayout_,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(PushConstantPBR), &pc_);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout_,
		0u, // firstSet
		1u, // descriptorSetCount
		&(descriptorSets_[frameIndex]),
		0u, // dynamicOffsetCount
		nullptr); // pDynamicOffsets

	vkCmdDrawIndirect(
		commandBuffer, 
		indirectBuffers_[frameIndex].buffer_, 
		0, // offset
		scene_->GetMeshCount(),
		sizeof(VkDrawIndirectCommand));
	
	vkCmdEndRenderPass(commandBuffer);
}

// TODO Refactor VulkanDescriptor to make the code below simpler
void PipelinePBRBindless::CreateDescriptor(VulkanContext& ctx)
{
	std::vector<VkDescriptorImageInfo> imageInfos = scene_->GetImageInfos(); // 9
	
	const uint32_t textureCount = static_cast<uint32_t>(imageInfos.size());
	constexpr uint32_t frameCount = AppConfig::FrameOverlapCount;

	VkDescriptorBufferInfo vertexBufferInfo = scene_->vertexBuffer_.GetBufferInfo(); // 2
	VkDescriptorBufferInfo indexBufferInfo = scene_->indexBuffer_.GetBufferInfo(); // 3
	VkDescriptorBufferInfo meshBufferInfo = scene_->meshDataBuffer_.GetBufferInfo(); // 4
	VkDescriptorBufferInfo lightBufferInfo = lights_->GetBufferInfo(); // 5
	VkDescriptorImageInfo specularImageInfo = iblResources_->specularCubemap_.GetDescriptorImageInfo(); // 6
	VkDescriptorImageInfo diffuseImageInfo = iblResources_->diffuseCubemap_.GetDescriptorImageInfo(); // 7
	VkDescriptorImageInfo lutImageInfo = iblResources_->brdfLut_.GetDescriptorImageInfo(); // 8

	constexpr size_t numWrites = 10;
	std::vector<DescriptorSetWrite> writes(numWrites);
	writes[0] = {.bufferInfoPtr_ = nullptr, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT };
	writes[1] = {.bufferInfoPtr_ = nullptr, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT };
	writes[2] = {.bufferInfoPtr_ = &vertexBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT };
	writes[3] = {.bufferInfoPtr_ = &indexBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT };
	writes[4] = {.bufferInfoPtr_ = &meshBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT };
	writes[5] = {.bufferInfoPtr_ = &lightBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .shaderFlags_ = VK_SHADER_STAGE_FRAGMENT_BIT };
	writes[6] = {.imageInfoPtr_ = &specularImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .shaderFlags_ = VK_SHADER_STAGE_FRAGMENT_BIT };
	writes[7] = {.imageInfoPtr_ = &diffuseImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .shaderFlags_ = VK_SHADER_STAGE_FRAGMENT_BIT };
	writes[8] = {.imageInfoPtr_ = &lutImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .shaderFlags_ = VK_SHADER_STAGE_FRAGMENT_BIT };
	writes[9] = {
		.imageInfoPtr_ = imageInfos.data(),
		.descriptorCount_ = textureCount,
		.type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.shaderFlags_ = VK_SHADER_STAGE_FRAGMENT_BIT };

	// Create pool and layout
	descriptor_.CreatePoolAndLayout(ctx, writes, frameCount, 1u);

	// Create sets
	descriptorSets_.resize(frameCount);
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		VkDescriptorBufferInfo camBufferInfo = cameraUBOBuffers_[i].GetBufferInfo(); // 0
		VkDescriptorBufferInfo modelBufferInfo = scene_->modelUBOBuffers_[i].GetBufferInfo(); // 1

		writes[0].bufferInfoPtr_ = &camBufferInfo;
		writes[1].bufferInfoPtr_ = &modelBufferInfo;

		descriptor_.CreateSet(ctx, writes, &(descriptorSets_[i]));
	}
}