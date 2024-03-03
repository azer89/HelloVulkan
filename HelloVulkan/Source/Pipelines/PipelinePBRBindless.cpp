#include "PipelinePBRBindless.h"
#include "VulkanUtility.h"
#include "DescriptorBuildInfo.h"
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
	ResourcesIBL* iblResources,
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
	constexpr uint32_t frameCount = AppConfig::FrameOverlapCount;

	DescriptorBuildInfo buildInfo;
	buildInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 0
	buildInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 1
	buildInfo.AddBuffer(&(scene_->vertexBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 2
	buildInfo.AddBuffer(&(scene_->indexBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 3
	buildInfo.AddBuffer(&(scene_->meshDataBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 4
	buildInfo.AddBuffer(lights_->GetVulkanBufferPtr(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT); // 5
	buildInfo.AddImage(&(iblResources_->specularCubemap_)); // 6
	buildInfo.AddImage(&(iblResources_->diffuseCubemap_)); // 7
	buildInfo.AddImage(&(iblResources_->brdfLut_)); // 8
	buildInfo.AddImageArray(scene_->GetImageInfos()); // 9

	// Pool and layout
	descriptor_.CreatePoolAndLayout(ctx, buildInfo, frameCount, 1u);

	// Sets
	descriptorSets_.resize(frameCount);
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		buildInfo.UpdateBuffer(&(cameraUBOBuffers_[i]), 0);
		buildInfo.UpdateBuffer(&(scene_->modelSSBOBuffers_[i]), 1);
		descriptor_.CreateSet(ctx, buildInfo, &(descriptorSets_[i]));
	}
}