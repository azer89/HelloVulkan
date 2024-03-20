#include "PipelinePBRBindless.h"
#include "VulkanUtility.h"
#include "VulkanDescriptorInfo.h"
#include "ResourcesShared.h"
#include "ResourcesLight.h"
#include "ResourcesIBL.h"
#include "Scene.h"
#include "Configs.h"

#include <vector>

// Constants
constexpr uint32_t UBO_COUNT = 2;
constexpr uint32_t SSBO_COUNT = 2;
constexpr uint32_t ENV_TEXTURE_COUNT = 3; // Specular, diffuse, and BRDF LUT

PipelinePBRBindless::PipelinePBRBindless(
	VulkanContext& ctx,
	Scene* scene,
	ResourcesLight* resLight,
	ResourcesIBL* iblResources,
	ResourcesShared* resShared,
	uint8_t renderBit) :
	PipelineBase(ctx, 
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.msaaSamples_ = resShared->multiSampledColorImage_.multisampleCount_,
			.vertexBufferBind_ = false,
		}
	),
	scene_(scene),
	resLight_(resLight),
	iblResources_(iblResources)
{
	VulkanBuffer::CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(CameraUBO), AppConfig::FrameCount);
	PrepareVIM(ctx); // Buffer device address
	CreateDescriptor(ctx);
	renderPass_.CreateOffScreenRenderPass(ctx, renderBit, config_.msaaSamples_);
	framebuffer_.CreateResizeable(
		ctx, 
		renderPass_.GetHandle(), 
		{
			&(resShared->multiSampledColorImage_),
			&(resShared->depthImage_)
		}, 
		IsOffscreen());
	// Push constants and pipeline layout
	const std::vector<VkPushConstantRange> ranges =
	{{
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset = 0u,
		.size = sizeof(PushConstPBR),
	}};
	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_, ranges);
	CreateGraphicsPipeline(
		ctx,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "Bindless/Scene.vert",
			AppConfig::ShaderFolder + "Bindless/Scene.frag"
		},
		&pipeline_
	);
}

PipelinePBRBindless::~PipelinePBRBindless()
{
	vimBuffer_.Destroy();
}

void PipelinePBRBindless::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "PBR_Bindless", tracy::Color::LimeGreen);

	const uint32_t frameIndex = ctx.GetFrameIndex();
	renderPass_.BeginRenderPass(ctx, commandBuffer, framebuffer_.GetFramebuffer());

	BindPipeline(ctx, commandBuffer);

	vkCmdPushConstants(
		commandBuffer,
		pipelineLayout_,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(PushConstPBR), &pc_);

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
		scene_->indirectBuffers_[frameIndex].buffer_, 
		0, // offset
		scene_->GetInstanceCount(),
		sizeof(VkDrawIndirectCommand));
	
	vkCmdEndRenderPass(commandBuffer);
}

void PipelinePBRBindless::PrepareVIM(VulkanContext& ctx)
{
	VIM vim = scene_->GetVIM();
	VkDeviceSize vimSize = sizeof(VIM);
	vimBuffer_.CreateBuffer(
		ctx,
		vimSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	vimBuffer_.UploadBufferData(ctx, &vim, vimSize);
}

void PipelinePBRBindless::CreateDescriptor(VulkanContext& ctx)
{
	constexpr uint32_t frameCount = AppConfig::FrameCount;

	VulkanDescriptorInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 0
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 1
	dsInfo.AddBuffer(&vimBuffer_, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 2
	dsInfo.AddBuffer(resLight_->GetVulkanBufferPtr(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT); // 3
	dsInfo.AddImage(&(iblResources_->specularCubemap_)); // 4
	dsInfo.AddImage(&(iblResources_->diffuseCubemap_)); // 5
	dsInfo.AddImage(&(iblResources_->brdfLut_)); // 6
	dsInfo.AddImageArray(scene_->GetImageInfos()); // 7

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