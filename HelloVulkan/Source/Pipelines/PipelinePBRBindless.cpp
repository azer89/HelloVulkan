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

	CreateIndirectBuffers(ctx);

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
	std::vector<VkPushConstantRange> ranges =
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
			AppConfig::ShaderFolder + "Bindless//Mesh.vert",
			AppConfig::ShaderFolder + "Bindless//Mesh.frag"
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
	uint32_t frameIndex = ctx.GetFrameIndex();
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
		scene_->models_.size(),
		sizeof(VkDrawIndirectCommand));
	
	vkCmdEndRenderPass(commandBuffer);
}

void PipelinePBRBindless::CreateIndirectBuffers(VulkanContext& ctx)
{
	const uint32_t meshSize = scene_->GetMeshCount();
	const uint32_t indirectDataSize = meshSize * sizeof(VkDrawIndirectCommand);
	const size_t numFrames = AppConfig::FrameOverlapCount;

	const std::vector<uint32_t> meshVertexCountArray = scene_->GetMeshVertexCountArray();
	
	indirectBuffers_.resize(numFrames);
	for (size_t i = 0; i < numFrames; ++i)
	{
		indirectBuffers_[i].CreateIndirectBuffer(ctx, indirectDataSize);
		VkDrawIndirectCommand* data = indirectBuffers_[i].MapIndirectBuffer();

		for (uint32_t j = 0; j < meshSize; ++j)
		{
			data[i] =
			{
				.vertexCount = static_cast<uint32_t>(meshVertexCountArray[j]),
				.instanceCount = 1u,
				.firstVertex = 0,
				.firstInstance = j
			};
		}

		// Unmap
		indirectBuffers_[i].UnmapIndirectBuffer();
	}
}

// TODO Refactor VulkanDescriptor to make the code below simpler
void PipelinePBRBindless::CreateDescriptor(VulkanContext& ctx)
{
	/*9*///std::vector<VkDescriptorImageInfo> imageInfos = scene_->GetImageInfos() ;
	std::vector<VkDescriptorImageInfo> imageInfos;
	for (size_t i = 0; i < scene_->models_.size(); ++i)
	{
		for (size_t j = 0; j < scene_->models_[i].textureList_.size(); ++j)
		{
			//imageInfos.push_back(scene_->models_[i].textureList_[j].GetDescriptorImageInfo());
			imageInfos.push_back(iblResources_->brdfLut_.GetDescriptorImageInfo());
		}
	}
	
	const uint32_t textureCount = static_cast<uint32_t>(imageInfos.size());
	const uint32_t frameCount = AppConfig::FrameOverlapCount;

	// Pool
	descriptor_.CreatePool(
		ctx,
		{
			.uboCount_ = UBO_COUNT,
			.ssboCount_ = SSBO_COUNT,
			.samplerCount_ = (textureCount + PBR_ENV_TEXTURE_COUNT),
			.frameCount_ = AppConfig::FrameOverlapCount,
			.setCountPerFrame_ = 1,
		});

	// Layout
	descriptor_.CreateLayout(ctx,
	{
		{
			.type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = 1
		},
		{
			.type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = 5
		},
		{
			.type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.shaderFlags_ = VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = PBR_ENV_TEXTURE_COUNT
		},
		{
			// NOTE Unbounded array should be the last
			.type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.shaderFlags_ = VK_SHADER_STAGE_FRAGMENT_BIT,
			.descriptorCount_ = textureCount,
			.bindingCount_ = 1u
		}
	});

	// Set
	/*2*/ VkDescriptorBufferInfo vertexBufferInfo = { scene_->vertexBuffer_.buffer_, 0, scene_->vertexBufferSize_};
	/*3*/ VkDescriptorBufferInfo indexBufferInfo = { scene_->indexBuffer_.buffer_, 0, scene_->indexBufferSize_ };
	/*4*/ VkDescriptorBufferInfo meshBufferInfo = { scene_->meshDataBuffer_.buffer_, 0, scene_->meshDataBufferSize_ };
	/*5*/ VkDescriptorBufferInfo lightBufferInfo = { lights_->GetSSBOBuffer(), 0, lights_->GetSSBOSize() };
	/*6*/ VkDescriptorImageInfo specularImageInfo = iblResources_->specularCubemap_.GetDescriptorImageInfo();
	/*7*/ VkDescriptorImageInfo diffuseImageInfo = iblResources_->diffuseCubemap_.GetDescriptorImageInfo();
	/*8*/ VkDescriptorImageInfo lutImageInfo = iblResources_->brdfLut_.GetDescriptorImageInfo();

	descriptorSets_.resize(frameCount);
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		/*0*/ VkDescriptorBufferInfo camBufferInfo = { cameraUBOBuffers_[i].buffer_, 0, sizeof(CameraUBO) };
		/*1*/ VkDescriptorBufferInfo modelBufferInfo = { scene_->modelUBOBuffers_[i].buffer_, 0, sizeof(ModelUBO) };

		std::vector<DescriptorSetWrite> writes;
		/*0*/ writes.push_back({ .bufferInfoPtr_ = &camBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });
		/*1*/ writes.push_back({ .bufferInfoPtr_ = &modelBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*2*/ writes.push_back({ .bufferInfoPtr_ = &vertexBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*3*/ writes.push_back({ .bufferInfoPtr_ = &indexBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*4*/ writes.push_back({ .bufferInfoPtr_ = &meshBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*5*/ writes.push_back({ .bufferInfoPtr_ = &lightBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*6*/ writes.push_back({ .imageInfoPtr_ = &specularImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		/*7*/ writes.push_back({ .imageInfoPtr_ = &diffuseImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		/*8*/ writes.push_back({ .imageInfoPtr_ = &lutImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		/*9*/ writes.push_back({
			.imageInfoPtr_ = imageInfos.data(),
			.descriptorCount_ = textureCount,
			.type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
	
		descriptor_.CreateSet(ctx, writes, &(descriptorSets_[i]));
	}
}