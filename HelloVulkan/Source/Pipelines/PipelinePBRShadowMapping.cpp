#include "PipelinePBRShadowMapping.h"
#include "VulkanUtility.h"
#include "Configs.h"

#include <vector>

// Constants
constexpr uint32_t UBO_COUNT = 2;
constexpr uint32_t SSBO_COUNT = 5;
//constexpr uint32_t PBR_MESH_TEXTURE_COUNT = 6;
constexpr uint32_t ENV_TEXTURE_COUNT = 4; // Specular, diffuse, BRDF LUT, and shadow map

PipelinePBRShadowMapping::PipelinePBRShadowMapping(
	VulkanContext& ctx,
	//std::vector<Model*> models,
	Scene* scene,
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
			.vertexBufferBind_ = false,
		}
	),
	//models_(models),
	scene_(scene),
	lights_(lights),
	iblResources_(iblResources),
	shadowMap_(shadowMap)
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

	for (auto& buffer : indirectBuffers_)
	{
		buffer.Destroy();
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

	/*size_t meshIndex = 0;
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
	}*/
	
	vkCmdEndRenderPass(commandBuffer);
}

void PipelinePBRShadowMapping::CreateDescriptor(VulkanContext& ctx)
{
	/*11*/ std::vector<VkDescriptorImageInfo> imageInfos = scene_->GetImageInfos();

	const uint32_t textureCount = static_cast<uint32_t>(imageInfos.size());
	constexpr uint32_t frameCount = AppConfig::FrameOverlapCount;

	// Pool
	descriptor_.CreatePool(
		ctx,
		{
			.uboCount_ = UBO_COUNT,
			.ssboCount_ = SSBO_COUNT,
			.samplerCount_ = (textureCount + ENV_TEXTURE_COUNT),
			.frameCount_ = frameCount,
			.setCountPerFrame_ = 1,
		});

	// Layout
	descriptor_.CreateLayout(ctx,
	{
		{
			.type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = 2u
		},
		{
			.type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = 5u
		},
		{
			.type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.shaderFlags_ = VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = ENV_TEXTURE_COUNT
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
	/*3*/ VkDescriptorBufferInfo vertexBufferInfo = { scene_->vertexBuffer_.buffer_, 0, scene_->vertexBufferSize_ };
	/*4*/ VkDescriptorBufferInfo indexBufferInfo = { scene_->indexBuffer_.buffer_, 0, scene_->indexBufferSize_ };
	/*5*/ VkDescriptorBufferInfo meshBufferInfo = { scene_->meshDataBuffer_.buffer_, 0, scene_->meshDataBufferSize_ };
	/*6*/ VkDescriptorBufferInfo lightBufferInfo = { lights_->GetSSBOBuffer(), 0, lights_->GetSSBOSize() };
	/*7*/ VkDescriptorImageInfo specularImageInfo = iblResources_->specularCubemap_.GetDescriptorImageInfo();
	/*8*/ VkDescriptorImageInfo diffuseImageInfo = iblResources_->diffuseCubemap_.GetDescriptorImageInfo();
	/*9*/ VkDescriptorImageInfo lutImageInfo = iblResources_->brdfLut_.GetDescriptorImageInfo();
	/*10*/ VkDescriptorImageInfo shadowImageInfo = shadowMap_->GetDescriptorImageInfo();

	descriptorSets_.resize(frameCount);
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		/*0*/ VkDescriptorBufferInfo camBufferInfo = { cameraUBOBuffers_[i].buffer_, 0, sizeof(CameraUBO) };
		/*1*/ VkDescriptorBufferInfo shadowUBOBufferInfo = { shadowMapConfigUBOBuffers_[i].buffer_, 0, sizeof(ShadowMapUBO) };
		/*2*/ VkDescriptorBufferInfo modelBufferInfo = { scene_->modelUBOBuffers_[i].buffer_, 0, scene_->modelUBOBufferSize_ };
		
		std::vector<DescriptorSetWrite> writes;
		/*0*/ writes.push_back({ .bufferInfoPtr_ = &camBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });
		/*1*/ writes.push_back({ .bufferInfoPtr_ = &shadowUBOBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });
		/*2*/ writes.push_back({ .bufferInfoPtr_ = &modelBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*3*/ writes.push_back({ .bufferInfoPtr_ = &vertexBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*4*/ writes.push_back({ .bufferInfoPtr_ = &indexBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*5*/ writes.push_back({ .bufferInfoPtr_ = &meshBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*6*/ writes.push_back({ .bufferInfoPtr_ = &lightBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*7*/ writes.push_back({ .imageInfoPtr_ = &specularImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		/*8*/ writes.push_back({ .imageInfoPtr_ = &diffuseImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		/*9*/ writes.push_back({ .imageInfoPtr_ = &lutImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		/*10*/ writes.push_back({ .imageInfoPtr_ = &shadowImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		/*11*/ writes.push_back({
			.imageInfoPtr_ = imageInfos.data(),
			.descriptorCount_ = textureCount,
			.type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });

		descriptor_.CreateSet(ctx, writes, &(descriptorSets_[i]));
	}

	/*uint32_t numMeshes = 0u;
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
	}*/
}

/*void PipelinePBRShadowMapping::CreateDescriptorSet(
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
}*/