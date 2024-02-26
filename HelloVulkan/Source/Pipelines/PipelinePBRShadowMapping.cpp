#include "PipelinePBRShadowMapping.h"
#include "VulkanUtility.h"
#include "Configs.h"

#include <vector>

// Constants
constexpr uint32_t UBO_COUNT = 3;
constexpr uint32_t SSBO_COUNT = 1;
constexpr uint32_t PBR_MESH_TEXTURE_COUNT = 6;
constexpr uint32_t PBR_ENV_TEXTURE_COUNT = 4; // Specular, diffuse, BRDF LUT, and shadow map

PipelinePBRShadowMapping::PipelinePBRShadowMapping(
	VulkanContext& ctx,
	std::vector<Model*> models,
	Lights* lights,
	IBLResources* iblResources,
	VulkanImage* shadowMap,
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
	iblResources_(iblResources),
	shadowMap_(shadowMap)
{
	// UBOs
	CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(CameraUBO), AppConfig::FrameOverlapCount);
	CreateMultipleUniformBuffers(ctx, shadowMapConfigUBOBuffers_, sizeof(ShadowMapUBO), AppConfig::FrameOverlapCount);

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

	CreateDescriptor(ctx);

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
			AppConfig::ShaderFolder + "ShadowMapping//Mesh.vert",
			AppConfig::ShaderFolder + "ShadowMapping//Mesh.frag"
		},
		&pipeline_
	);
}

PipelinePBRShadowMapping::~PipelinePBRShadowMapping()
{
	for (auto uboBuffer : shadowMapConfigUBOBuffers_)
	{
		uboBuffer.Destroy();
	}
}

void PipelinePBRShadowMapping::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
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

	size_t meshIndex = 0;
	for (Model* model : models_)
	{
		for (Mesh& mesh : model->meshes_)
		{
			vkCmdBindDescriptorSets(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout_,
				0,
				1,
				&(descriptorSets_[meshIndex++][frameIndex]),
				0,
				nullptr);

			// Bind vertex buffer
			VkBuffer buffers[] = { mesh.vertexBuffer_.buffer_ };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

			// Bind index buffer
			vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer_.buffer_, 0, VK_INDEX_TYPE_UINT32);

			// Draw
			vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.indexBufferSize_ / (sizeof(unsigned int))), 1, 0, 0, 0);
		}
	}
	
	vkCmdEndRenderPass(commandBuffer);
}

void PipelinePBRShadowMapping::CreateDescriptor(VulkanContext& ctx)
{
	uint32_t numMeshes = 0u;
	for (Model* model : models_)
	{
		numMeshes += model->NumMeshes();
	}

	// Pool
	descriptor_.CreatePool(
		ctx,
		{
			.uboCount_ = UBO_COUNT,
			.ssboCount_ = SSBO_COUNT,
			.samplerCount_ = PBR_MESH_TEXTURE_COUNT + PBR_ENV_TEXTURE_COUNT,
			.frameCount_ = AppConfig::FrameOverlapCount,
			.setCountPerFrame_ = numMeshes,
		});

	// Layout
	descriptor_.CreateLayout(ctx,
	{
		{
			.type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = 3
		},
		{
			.type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = 1
		},
		{
			.type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.shaderFlags_ = VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = PBR_MESH_TEXTURE_COUNT + PBR_ENV_TEXTURE_COUNT
		}
	});

	// Set
	descriptorSets_.resize(numMeshes);
	size_t meshIndex = 0;
	for (Model* model : models_)
	{
		for (Mesh& mesh : model->meshes_)
		{
			CreateDescriptorSet(ctx, model, &mesh, meshIndex++);
		}
	}
}

void PipelinePBRShadowMapping::CreateDescriptorSet(
	VulkanContext& ctx, 
	Model* parentModel, 
	Mesh* mesh, 
	const size_t meshIndex)
{
	VkDescriptorImageInfo specularImageInfo = iblResources_->specularCubemap_.GetDescriptorImageInfo();
	VkDescriptorImageInfo diffuseImageInfo = iblResources_->diffuseCubemap_.GetDescriptorImageInfo();
	VkDescriptorImageInfo lutImageInfo = iblResources_->brdfLut_.GetDescriptorImageInfo();
	VkDescriptorImageInfo shadowImageInfo = shadowMap_->GetDescriptorImageInfo();

	// PBR Textures
	std::vector<VkDescriptorImageInfo> meshTextureInfos(PBR_MESH_TEXTURE_COUNT);
	for (const auto& elem : mesh->textureIndices_)
	{
		// Should be ordered based on elem.first
		uint32_t typeIndex = static_cast<uint32_t>(elem.first) - 1;
		int textureIndex = elem.second;
		VulkanImage* texture = parentModel->GetTexture(textureIndex);
		meshTextureInfos[typeIndex] = texture->GetDescriptorImageInfo();
	}

	size_t frameCount = AppConfig::FrameOverlapCount;
	descriptorSets_[meshIndex].resize(frameCount);

	for (size_t i = 0; i < frameCount; i++)
	{
		VkDescriptorBufferInfo bufferInfo1 = { cameraUBOBuffers_[i].buffer_, 0, sizeof(CameraUBO) };
		VkDescriptorBufferInfo bufferInfo2 = { parentModel->modelBuffers_[i].buffer_, 0, sizeof(ModelUBO) };
		VkDescriptorBufferInfo bufferInfo3 = { shadowMapConfigUBOBuffers_[i].buffer_, 0, sizeof(ShadowMapUBO) };
		VkDescriptorBufferInfo bufferInfo4 = { lights_->GetSSBOBuffer(), 0, lights_->GetSSBOSize() };
		
		std::vector<DescriptorSetWrite> writes;

		writes.push_back({ .bufferInfoPtr_ = &bufferInfo1, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });
		writes.push_back({ .bufferInfoPtr_ = &bufferInfo2, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });
		writes.push_back({ .bufferInfoPtr_ = &bufferInfo3, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });
		writes.push_back({ .bufferInfoPtr_ = &bufferInfo4, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		for (auto& textureInfo : meshTextureInfos)
		{
			writes.push_back({ .imageInfoPtr_ = &textureInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		}
		writes.push_back({ .imageInfoPtr_ = &specularImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		writes.push_back({ .imageInfoPtr_ = &diffuseImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		writes.push_back({ .imageInfoPtr_ = &lutImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		writes.push_back({ .imageInfoPtr_ = &shadowImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });

		descriptor_.CreateSet(ctx, writes, &(descriptorSets_[meshIndex][i]));
	}
}