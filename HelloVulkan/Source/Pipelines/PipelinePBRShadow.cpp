#include "PipelinePBRShadow.h"
#include "ResourcesLight.h"
#include "ResourcesShadow.h"
#include "ResourcesShared.h"
#include "ResourcesIBL.h"
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
	ResourcesLight* resourcesLight,
	ResourcesIBL* resourcesIBL,
	ResourcesShadow* resourcesShadow,
	ResourcesShared* resourcesShared,
	ResourcesGBuffer* resourcesGBuffer,
	MaterialType materialType,
	uint8_t renderBit) :
	PipelineBase(ctx, 
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.msaaSamples_ = resourcesShared->multiSampledColorImage_.multisampleCount_,
			.vertexBufferBind_ = false, // If you use bindless texture, make sure this is false
		}
	),
	scene_(scene),
	resourcesLight_(resourcesLight),
	resourcesIBL_(resourcesIBL),
	resourcesShadow_(resourcesShadow),
	resourcesGBuffer_(resourcesGBuffer),
	materialType_(materialType),
	materialOffset_(0),
	materialDrawCount_(0)
{
	VulkanBuffer::CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(CameraUBO), AppConfig::FrameCount);
	VulkanBuffer::CreateMultipleUniformBuffers(ctx, shadowMapConfigUBOBuffers_, sizeof(ShadowMapUBO), AppConfig::FrameCount);

	scene_->GetOffsetAndDrawCount(materialType_, materialOffset_, materialDrawCount_);

	renderPass_.CreateOffScreen(ctx, renderBit, config_.msaaSamples_);
	framebuffer_.CreateResizeable(
		ctx, 
		renderPass_.GetHandle(), 
		{
			&(resourcesShared->multiSampledColorImage_),
			&(resourcesShared->depthImage_)
		}, 
		IsOffscreen());
	CreateBDABuffer(ctx);
	CreateDescriptor(ctx);
	CreatePipelineLayout(ctx, descriptorManager_.layout_, &pipelineLayout_, sizeof(PushConstPBR), VK_SHADER_STAGE_FRAGMENT_BIT);
	CreateSpecializationConstants();
	CreateGraphicsPipeline(
		ctx,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "ShadowMapping/Scene.vert",
			AppConfig::ShaderFolder + "ShadowMapping/Scene.frag"
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
	bdaBuffer_.Destroy();
}

void PipelinePBRShadow::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	if (materialDrawCount_ == 0)
	{
		return;
	}

	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "PBR_Shadow", tracy::Color::Aqua);
	
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
		0u,
		1u,
		&(descriptorSets_[frameIndex]),
		0u,
		nullptr);

	ctx.InsertDebugLabel(commandBuffer, "PipelinePBRShadow", 0xff9999ff);

	vkCmdDrawIndirect(
		commandBuffer,
		scene_->indirectBuffer_.buffer_,
		materialOffset_,
		materialDrawCount_,
		sizeof(VkDrawIndirectCommand));

	vkCmdEndRenderPass(commandBuffer);
}

void PipelinePBRShadow::CreateSpecializationConstants()
{
	alphaDiscard_ = materialType_ == MaterialType::Transparent ? 1u : 0u;

	std::vector<VkSpecializationMapEntry> specializationEntries = {{
		.constantID = 0,
		.offset = 0,
		.size = sizeof(uint32_t)
	}};

	specializationConstants_.ConsumeEntries(
		std::move(specializationEntries),
		&alphaDiscard_,
		sizeof(uint32_t),
		VK_SHADER_STAGE_FRAGMENT_BIT);
}

void PipelinePBRShadow::CreateBDABuffer(VulkanContext& ctx)
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

void PipelinePBRShadow::OnWindowResized(VulkanContext& ctx)
{
	PipelineBase::OnWindowResized(ctx);
	UpdateDescriptorSets(ctx);
}

void PipelinePBRShadow::CreateDescriptor(VulkanContext& ctx)
{
	descriptorSetInfo_.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 0
	descriptorSetInfo_.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 1
	descriptorSetInfo_.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 2
	descriptorSetInfo_.AddBuffer(&bdaBuffer_, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 3
	descriptorSetInfo_.AddBuffer(resourcesLight_->GetVulkanBufferPtr(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 4
	descriptorSetInfo_.AddImage(&(resourcesIBL_->specularCubemap_)); // 5
	descriptorSetInfo_.AddImage(&(resourcesIBL_->diffuseCubemap_)); // 6
	descriptorSetInfo_.AddImage(&(resourcesIBL_->brdfLut_)); // 7
	descriptorSetInfo_.AddImage(&(resourcesShadow_->shadowMap_)); // 8
	descriptorSetInfo_.AddImage(nullptr); // 9
	descriptorSetInfo_.AddImageArray(scene_->GetImageInfos()); // 10

	// Pool and layout
	descriptorManager_.CreatePoolAndLayout(ctx, descriptorSetInfo_, AppConfig::FrameCount, 1u);

	AllocateDescriptorSets(ctx);
	UpdateDescriptorSets(ctx);
}

void PipelinePBRShadow::AllocateDescriptorSets(VulkanContext& ctx)
{
	for (uint32_t i = 0; i < AppConfig::FrameCount; ++i)
	{
		descriptorManager_.AllocateSet(ctx, &(descriptorSets_[i]));
	}
}

void PipelinePBRShadow::UpdateDescriptorSets(VulkanContext& ctx)
{
	descriptorSetInfo_.UpdateImage(&(resourcesGBuffer_->ssao_), 9); // 9
	for (uint32_t i = 0; i < AppConfig::FrameCount; ++i)
	{
		// Need to update all double-buffered resources because 
		// VulkanDescriptorSetInfo::writes_ itself is not double buffered
		descriptorSetInfo_.UpdateBuffer(&(cameraUBOBuffers_[i]), 0);
		descriptorSetInfo_.UpdateBuffer(&(shadowMapConfigUBOBuffers_[i]), 1);
		descriptorSetInfo_.UpdateBuffer(&(scene_->modelSSBOBuffers_[i]), 2);
		descriptorManager_.UpdateSet(ctx, descriptorSetInfo_, &(descriptorSets_[i]));
	}
}