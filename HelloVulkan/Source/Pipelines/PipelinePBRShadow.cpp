#include "PipelinePBRShadow.h"
#include "ResourcesLight.h"
#include "ResourcesShadow.h"
#include "ResourcesShared.h"
#include "ResourcesIBL.h"
#include "VulkanUtility.h"
#include "Scene.h"
#include "Configs.h"

#include <vector>

// Constants
constexpr uint32_t UBO_COUNT = 2;
constexpr uint32_t SSBO_COUNT = 5;
constexpr uint32_t ENV_TEXTURE_COUNT = 4; // Specular, diffuse, BRDF LUT, and shadow map

PipelinePBRShadow::PipelinePBRShadow(
	VulkanContext& ctx,
	Scene* scene,
	ResourcesLight* resLight,
	ResourcesIBL* iblResources,
	ResourcesShadow* resShadow,
	ResourcesShared* resShared,
	uint8_t renderBit) :
	PipelineBase(ctx, 
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.msaaSamples_ = resShared->multiSampledColorImage_.multisampleCount_,

			// If you use bindless texture, make sure this is false
			.vertexBufferBind_ = false,
		}
	),
	scene_(scene),
	resLight_(resLight),
	iblResources_(iblResources),
	resShadow_(resShadow)
{
	// UBOs
	CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(CameraUBO), AppConfig::FrameOverlapCount);
	CreateMultipleUniformBuffers(ctx, shadowMapConfigUBOBuffers_, sizeof(ShadowMapUBO), AppConfig::FrameOverlapCount);

	CreateIndirectBuffers(ctx, scene_, indirectBuffers_);

	// Note that this pipeline is offscreen rendering
	renderPass_.CreateOffScreenRenderPass(ctx, renderBit, config_.msaaSamples_);

	framebuffer_.CreateResizeable(
		ctx, 
		renderPass_.GetHandle(), 
		{
			&(resShared->multiSampledColorImage_),
			&(resShared->depthImage_)
		}, 
		IsOffscreen());

	PrepareVIM(ctx);

	CreateDescriptor(ctx);

	// Push constants
	std::vector<VkPushConstantRange> ranges =
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
			AppConfig::ShaderFolder + "ShadowMapping//Scene.vert",
			AppConfig::ShaderFolder + "ShadowMapping//Scene.frag"
		},
		&pipeline_
	);
}

PipelinePBRShadow::~PipelinePBRShadow()
{
	for (auto uboBuffer : shadowMapConfigUBOBuffers_)
	{
		uboBuffer.Destroy();
	}

	for (auto& buffer : indirectBuffers_)
	{
		buffer.Destroy();
	}

	vimBuffer_.Destroy();
}

void PipelinePBRShadow::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "PBR_Shadow_Mapping", tracy::Color::Aquamarine);

	uint32_t frameIndex = ctx.GetFrameIndex();
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
		0u,
		1u,
		&(descriptorSets_[frameIndex]),
		0u,
		nullptr);

	vkCmdDrawIndirect(
		commandBuffer,
		indirectBuffers_[frameIndex].buffer_,
		0, // offset
		scene_->GetMeshCount(),
		sizeof(VkDrawIndirectCommand));
	
	vkCmdEndRenderPass(commandBuffer);
}

void PipelinePBRShadow::PrepareVIM(VulkanContext& ctx)
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

void PipelinePBRShadow::CreateDescriptor(VulkanContext& ctx)
{
	constexpr uint32_t frameCount = AppConfig::FrameOverlapCount;
	VulkanDescriptorInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 0
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 1
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 2

	dsInfo.AddBuffer(&vimBuffer_, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 3
	dsInfo.AddBuffer(resLight_->GetVulkanBufferPtr(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 4

	dsInfo.AddImage(&(iblResources_->specularCubemap_)); // 5
	dsInfo.AddImage(&(iblResources_->diffuseCubemap_)); // 6
	dsInfo.AddImage(&(iblResources_->brdfLut_)); // 7
	dsInfo.AddImage(&(resShadow_->shadowMap_)); // 8
	dsInfo.AddImageArray(scene_->GetImageInfos()); // 9

	// Pool and layout
	descriptor_.CreatePoolAndLayout(ctx, dsInfo, frameCount, 1u);

	// Sets
	descriptorSets_.resize(frameCount);
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(cameraUBOBuffers_[i]), 0);
		dsInfo.UpdateBuffer(&(shadowMapConfigUBOBuffers_[i]), 1);
		dsInfo.UpdateBuffer(&(scene_->modelSSBOBuffers_[i]), 2);
		descriptor_.CreateSet(ctx, dsInfo, &(descriptorSets_[i]));
	}
}