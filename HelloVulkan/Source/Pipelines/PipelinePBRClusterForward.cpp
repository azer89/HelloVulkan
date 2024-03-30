#include "PipelinePBRClusterForward.h"
#include "ResourcesClusterForward.h"
#include "ResourcesShared.h"
#include "ResourcesLight.h"
#include "ResourcesIBL.h"
#include "Scene.h"
#include "Configs.h"

// Constants
constexpr uint32_t UBO_COUNT = 3;
constexpr uint32_t SSBO_COUNT = 3;
constexpr uint32_t PBR_TEXTURE_COUNT = 6;
constexpr uint32_t ENV_TEXTURE_COUNT = 3; // Specular, diffuse, and BRDF LUT

PipelinePBRClusterForward::PipelinePBRClusterForward(
	VulkanContext& ctx,
	Scene* scene,
	ResourcesLight* lights,
	ResourcesClusterForward* resourcesCF,
	ResourcesIBL* resourcesIBL,
	ResourcesShared* resourcesShared,
	MaterialType materialType,
	uint8_t renderBit) :
	PipelineBase(ctx,
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.msaaSamples_ = resourcesShared->multiSampledColorImage_.multisampleCount_,
			.vertexBufferBind_ = false,
		}),
	scene_(scene),
	resourcesLight_(lights),
	resourcesCF_(resourcesCF),
	resourcesIBL_(resourcesIBL),
	materialType_(materialType),
	materialOffset_(0),
	materialDrawCount_(0)
{
	VulkanBuffer::CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(CameraUBO), AppConfig::FrameCount);
	VulkanBuffer::CreateMultipleUniformBuffers(ctx, cfUBOBuffers_, sizeof(ClusterForwardUBO), AppConfig::FrameCount);

	scene_->GetOffsetAndDrawCount(materialType_, materialOffset_, materialDrawCount_);

	renderPass_.CreateOffScreenRenderPass(ctx, renderBit, config_.msaaSamples_);
	framebuffer_.CreateResizeable(
		ctx,
		renderPass_.GetHandle(),
		{
			&(resourcesShared->multiSampledColorImage_),
			&(resourcesShared->depthImage_)
		},
		IsOffscreen());
	PrepareBDA(ctx); // Buffer device address
	CreateDescriptor(ctx);
	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_, sizeof(PushConstPBR), VK_SHADER_STAGE_FRAGMENT_BIT);
	CreateSpecializationConstants();
	CreateGraphicsPipeline(
		ctx,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "Bindless/Scene.vert",
			AppConfig::ShaderFolder + "ClusteredForward/Scene.frag"
		},
		&pipeline_
	);
}

PipelinePBRClusterForward::~PipelinePBRClusterForward()
{
	for (auto uboBuffer : cfUBOBuffers_)
	{
		uboBuffer.Destroy();
	}
	bdaBuffer_.Destroy();
}

void PipelinePBRClusterForward::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "PBR_Cluster_Forward", tracy::Color::MediumPurple);

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

	ctx.InsertDebugLabel(commandBuffer, "PipelinePBRClusteredForward", 0xff9999ff);

	vkCmdDrawIndirect(
		commandBuffer,
		scene_->indirectBuffer_.buffer_,
		materialOffset_,
		materialDrawCount_,
		sizeof(VkDrawIndirectCommand));

	vkCmdEndRenderPass(commandBuffer);
}

void PipelinePBRClusterForward::PrepareBDA(VulkanContext& ctx)
{
	BDA bda = scene_->GetBDA();
	VkDeviceSize bdaSize = sizeof(BDA);
	bdaBuffer_.CreateBuffer(
		ctx,
		bdaSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU
	);
	bdaBuffer_.UploadBufferData(ctx, &bda, bdaSize);
}

void PipelinePBRClusterForward::CreateSpecializationConstants()
{
	alphaDiscard_ = materialType_ == MaterialType::Transparent ? 1u : 0u;

	std::vector<VkSpecializationMapEntry> specializationEntries = { {
		.constantID = 0,
		.offset = 0,
		.size = sizeof(uint32_t)
	} };

	specializationConstants_.ConsumeEntries(
		std::move(specializationEntries),
		&alphaDiscard_,
		sizeof(uint32_t),
		VK_SHADER_STAGE_FRAGMENT_BIT);
}

void PipelinePBRClusterForward::CreateDescriptor(VulkanContext& ctx)
{
	constexpr uint32_t frameCount = AppConfig::FrameCount;

	VulkanDescriptorInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 0
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 1
	dsInfo.AddBuffer(&bdaBuffer_, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 2
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER); // 3
	dsInfo.AddBuffer(resourcesLight_->GetVulkanBufferPtr(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT); // 4
	dsInfo.AddBuffer(&(resourcesCF_->lightCellsBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 5
	dsInfo.AddBuffer(&(resourcesCF_->lightIndicesBuffer_), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); // 6
	dsInfo.AddImage(&(resourcesIBL_->specularCubemap_)); // 7
	dsInfo.AddImage(&(resourcesIBL_->diffuseCubemap_)); // 8
	dsInfo.AddImage(&(resourcesIBL_->brdfLut_)); // 9
	dsInfo.AddImageArray(scene_->GetImageInfos()); // 10

	// Pool and layout
	descriptor_.CreatePoolAndLayout(ctx, dsInfo, frameCount, 1u);

	// Sets
	descriptorSets_.resize(frameCount);
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(cameraUBOBuffers_[i]), 0);
		dsInfo.UpdateBuffer(&(scene_->modelSSBOBuffers_[i]), 1);
		dsInfo.UpdateBuffer(&(cfUBOBuffers_[i]), 3);
		descriptor_.CreateSet(ctx, dsInfo, &(descriptorSets_[i]));
	}
}