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
	std::vector<Model*> models,
	Lights* lights,
	IBLResources* iblResources,
	VulkanImage* depthImage,
	VulkanImage* offscreenColorImage,
	uint8_t renderBit) :
	PipelineBase(ctx, 
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.msaaSamples_ = offscreenColorImage->multisampleCount_,
			.vertexBufferBind_ = true,
		}
	),
	models_(models),
	lights_(lights),
	iblResources_(iblResources)
{
	// Per frame UBO
	CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(CameraUBO), AppConfig::FrameOverlapCount);
	
	// Model UBO
	for (Model* model : models_)
	{
		CreateMultipleUniformBuffers(ctx, model->modelBuffers_, sizeof(ModelUBO), AppConfig::FrameOverlapCount);
	}

	// Model resources
	modelResources_.resize(models.size());
	CreateIndirectBuffers(ctx);
	for (size_t i = 0; i < models_.size(); ++i)
	{
		CreateDescriptor(ctx, i);
	}

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

	// Push constants
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
	for (PerModelBindlessResource& res : modelResources_)
	{
		res.Destroy();
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

	for (size_t i = 0; i < models_.size(); ++i)
	{
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout_,
			0u, // firstSet
			1u, // descriptorSetCount
			&(modelResources_[i].descriptorSets_[frameIndex]),
			0u, // dynamicOffsetCount
			nullptr); // pDynamicOffsets

		vkCmdDrawIndirect(
			commandBuffer, 
			modelResources_[i].indirectBuffers_[frameIndex].buffer_, 
			0, // offset
			models_[i]->NumMeshes(),
			sizeof(VkDrawIndirectCommand));
	}
	
	vkCmdEndRenderPass(commandBuffer);
}

void PipelinePBRBindless::CreateIndirectBuffers(VulkanContext& ctx)
{
	size_t numFrames = AppConfig::FrameOverlapCount;
	for (size_t i = 0; i < models_.size(); ++i)
	{
		const uint32_t meshSize = static_cast<uint32_t>(models_[i]->meshes_.size());
		const uint32_t indirectDataSize = meshSize * sizeof(VkDrawIndirectCommand);
		
		modelResources_[i].indirectBuffers_.resize(numFrames);
		for (size_t j = 0; j < numFrames; ++j)
		{
			// Create and map
			modelResources_[i].indirectBuffers_[j].CreateIndirectBuffer(ctx, indirectDataSize);
			VkDrawIndirectCommand* data = modelResources_[i].indirectBuffers_[j].MapIndirectBuffer();

			for (uint32_t k = 0; k < meshSize; ++k)
			{
				data[i] = 
				{
					.vertexCount = static_cast<uint32_t>(models_[i]->meshes_[k].vertices_.size()),
					.instanceCount = 1u,
					.firstVertex = 0,
					.firstInstance = k
				};
			}

			// Unmap
			modelResources_[i].indirectBuffers_[j].UnmapIndirectBuffer();
		}
	}
}

// TODO Refactor VulkanDescriptor to make the code below simpler
void PipelinePBRBindless::CreateDescriptor(VulkanContext& ctx, size_t modelIndex)
{
	Model* model = models_[modelIndex];
	const uint32_t textureCount = model->GetNumTextures();
	const uint32_t frameCount = AppConfig::FrameOverlapCount;

	// Pool
	modelResources_[modelIndex].descriptor_.CreatePool(
		ctx,
		{
			.uboCount_ = UBO_COUNT,
			.ssboCount_ = SSBO_COUNT,
			.samplerCount_ = (textureCount + PBR_ENV_TEXTURE_COUNT),
			.frameCount_ = AppConfig::FrameOverlapCount,
			.setCountPerFrame_ = 1,
		});

	// Layout
	modelResources_[modelIndex].descriptor_.CreateLayout(ctx,
	{
		{
			.type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = 2
		},
		{
			.type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = 4
		},
		{
			.type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.shaderFlags_ = VK_SHADER_STAGE_FRAGMENT_BIT,
			.descriptorCount_ = textureCount,
			.bindingCount_ = 1
		},
		{
			.type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.shaderFlags_ = VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = PBR_ENV_TEXTURE_COUNT
		}
	});

	// Set
	modelResources_[modelIndex].descriptorSets_.resize(frameCount);

	/*2*/ VkDescriptorBufferInfo vertexBufferInfo = { model->vertexBuffer_.buffer_, 0, model->vertexBufferSize_};
	/*3*/ VkDescriptorBufferInfo indexBufferInfo = { model->indexBuffer_.buffer_, 0, model->indexBufferSize_ };
	/*4*/ VkDescriptorBufferInfo meshBufferInfo = { model->meshDataBuffer_.buffer_, 0, model->meshDataBufferSize_ };
	/*5*/ VkDescriptorBufferInfo lightBufferInfo = { lights_->GetSSBOBuffer(), 0, lights_->GetSSBOSize() };
	/*6*/ std::vector<VkDescriptorImageInfo> textureInfoArray(textureCount);
	for (size_t i = 0; i < textureCount; ++i)
	{
		VulkanImage* texture = model->GetTexture(i);
		textureInfoArray[i] = texture->GetDescriptorImageInfo();
	}
	/*7*/ VkDescriptorImageInfo specularImageInfo = iblResources_->specularCubemap_.GetDescriptorImageInfo();
	/*8*/ VkDescriptorImageInfo diffuseImageInfo = iblResources_->diffuseCubemap_.GetDescriptorImageInfo();
	/*9*/ VkDescriptorImageInfo lutImageInfo = iblResources_->brdfLut_.GetDescriptorImageInfo();
	
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		/*0*/ VkDescriptorBufferInfo camBufferInfo = { cameraUBOBuffers_[i].buffer_, 0, sizeof(CameraUBO) };
		/*1*/ VkDescriptorBufferInfo modelBufferInfo = { model->modelBuffers_[i].buffer_, 0, sizeof(ModelUBO) };

		std::vector<DescriptorSetWrite> writes;
		/*0*/ writes.push_back({ .bufferInfoPtr_ = &camBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });
		/*1*/ writes.push_back({ .bufferInfoPtr_ = &modelBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });
		/*2*/ writes.push_back({ .bufferInfoPtr_ = &vertexBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*3*/ writes.push_back({ .bufferInfoPtr_ = &indexBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*4*/ writes.push_back({ .bufferInfoPtr_ = &meshBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*5*/ writes.push_back({ .bufferInfoPtr_ = &lightBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*6*/ writes.push_back({
			.imageInfoPtr_ = textureInfoArray.data(),
			.descriptorCount_ = textureCount,
			.type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		/*7*/ writes.push_back({ .imageInfoPtr_ = &specularImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		/*8*/ writes.push_back({ .imageInfoPtr_ = &diffuseImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		/*9*/ writes.push_back({ .imageInfoPtr_ = &lutImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
	
		modelResources_[modelIndex].descriptor_.CreateSet(ctx, writes, &(modelResources_[modelIndex].descriptorSets_[i]));
	}
}