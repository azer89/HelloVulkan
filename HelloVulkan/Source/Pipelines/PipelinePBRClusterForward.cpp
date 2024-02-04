#include "PipelinePBRClusterForward.h"
#include "Configs.h"

// Constants
constexpr uint32_t UBO_COUNT = 3;
constexpr uint32_t SSBO_COUNT = 4;
constexpr uint32_t PBR_MESH_TEXTURE_COUNT = 6;
constexpr uint32_t PBR_ENV_TEXTURE_COUNT = 3; // Specular, diffuse, and BRDF LUT

PipelinePBRClusterForward::PipelinePBRClusterForward(
	VulkanDevice& vkDev,
	std::vector<Model*> models,
	Lights* lights,
	ClusterForwardBuffers* cfBuffers,
	VulkanImage* specularMap,
	VulkanImage* diffuseMap,
	VulkanImage* brdfLUT,
	VulkanImage* depthImage,
	VulkanImage* offscreenColorImage,
	uint8_t renderBit) :
	cfBuffers_(cfBuffers),

	// Call the parent constructor
	PipelinePBR(
		vkDev,
		models,
		lights,
		specularMap,
		diffuseMap,
		brdfLUT,
		depthImage,
		offscreenColorImage,
		renderBit
	)
{
}

// Override
void PipelinePBRClusterForward::InitExtraResources(VulkanDevice& vkDev)
{
	CreateUniformBuffers(vkDev, cfUBOBuffers_, sizeof(ClusterForwardUBO));
}

// Override
void PipelinePBRClusterForward::CreatePBRPipeline(VulkanDevice& vkDev)
{
	CreateGraphicsPipeline(
		vkDev,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "Mesh.vert",
			AppConfig::ShaderFolder + "MeshClusterForward.frag"
		},
		&pipeline_
	);
}

// Override
void PipelinePBRClusterForward::CreateDescriptor(VulkanDevice& vkDev)
{
	uint32_t numMeshes = 0u;
	for (Model* model : models_)
	{
		numMeshes += model->NumMeshes();
	}

	// Pool
	descriptor_.CreatePool(
		vkDev,
		{
			.uboCount_ = UBO_COUNT * static_cast<uint32_t>(models_.size()),
			.ssboCount_ = SSBO_COUNT,
			.samplerCount_ = (PBR_MESH_TEXTURE_COUNT + PBR_ENV_TEXTURE_COUNT) * numMeshes,
			.swapchainCount_ = static_cast<uint32_t>(vkDev.GetSwapchainImageCount()),
			.setCountPerSwapchain_ = numMeshes,
		});

	// Layout
	descriptor_.CreateLayout(vkDev,
		{
			{
				.descriptorType_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				.bindingCount_ = 3
			},
			{
				.descriptorType_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				.bindingCount_ = 4
			},
			{
				.descriptorType_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.shaderFlags_ = VK_SHADER_STAGE_FRAGMENT_BIT,
				.bindingCount_ = PBR_MESH_TEXTURE_COUNT + PBR_ENV_TEXTURE_COUNT
			}
		});

	// Set
	for (Model* model : models_)
	{
		for (Mesh& mesh : model->meshes_)
		{
			CreateDescriptorSet(vkDev, model, mesh);
		}
	}
}

// Override
void PipelinePBRClusterForward::CreateDescriptorSet(VulkanDevice& vkDev, Model* parentModel, Mesh& mesh)
{
	VkDescriptorImageInfo specularImageInfo = specularCubemap_->GetDescriptorImageInfo();
	VkDescriptorImageInfo diffuseImageInfo = diffuseCubemap_->GetDescriptorImageInfo();
	VkDescriptorImageInfo lutImageInfo = brdfLUT_->GetDescriptorImageInfo();

	std::vector<VkDescriptorImageInfo> meshTextureInfos(PBR_MESH_TEXTURE_COUNT);
	for (const auto& elem : mesh.textures_)
	{
		// Should be ordered based on elem.first
		uint32_t index = static_cast<uint32_t>(elem.first) - 1;
		meshTextureInfos[index] = elem.second->GetDescriptorImageInfo();
	}

	size_t swapchainLength = vkDev.GetSwapchainImageCount();
	mesh.descriptorSets_.resize(swapchainLength);

	for (size_t i = 0; i < swapchainLength; i++)
	{
		VkDescriptorBufferInfo bufferInfo1 = { cameraUBOBuffers_[i].buffer_, 0, sizeof(CameraUBO) };
		VkDescriptorBufferInfo bufferInfo2 = { parentModel->modelBuffers_[i].buffer_, 0, sizeof(ModelUBO) };
		VkDescriptorBufferInfo bufferInfo3 = { cfUBOBuffers_[i].buffer_, 0, sizeof(ClusterForwardUBO) };
		VkDescriptorBufferInfo bufferInfo4 = { lights_->GetSSBOBuffer(), 0, lights_->GetSSBOSize() };
		VkDescriptorBufferInfo bufferInfo5 = { cfBuffers_->lightCellsBuffers_[i].buffer_, 0, VK_WHOLE_SIZE };
		VkDescriptorBufferInfo bufferInfo6 = { cfBuffers_->lightIndicesBuffers_[i].buffer_, 0, VK_WHOLE_SIZE };
		VkDescriptorBufferInfo bufferInfo7 = { cfBuffers_->aabbBuffers_[i].buffer_, 0, VK_WHOLE_SIZE};

		std::vector<DescriptorWrite> writes;

		writes.push_back({ .bufferInfoPtr_ = &bufferInfo1, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });
		writes.push_back({ .bufferInfoPtr_ = &bufferInfo2, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });
		writes.push_back({ .bufferInfoPtr_ = &bufferInfo3, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });
		writes.push_back({ .bufferInfoPtr_ = &bufferInfo4, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		writes.push_back({ .bufferInfoPtr_ = &bufferInfo5, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		writes.push_back({ .bufferInfoPtr_ = &bufferInfo6, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		writes.push_back({ .bufferInfoPtr_ = &bufferInfo7, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		for (size_t i = 0; i < meshTextureInfos.size(); ++i)
		{
			writes.push_back({ .imageInfoPtr_ = &meshTextureInfos[i], .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		}
		writes.push_back({ .imageInfoPtr_ = &specularImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		writes.push_back({ .imageInfoPtr_ = &diffuseImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		writes.push_back({ .imageInfoPtr_ = &lutImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });

		descriptor_.CreateSet(vkDev, writes, &(mesh.descriptorSets_[i]));
	}
}