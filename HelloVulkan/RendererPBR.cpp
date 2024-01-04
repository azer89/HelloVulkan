#include "RendererPBR.h"
#include "VulkanUtility.h"
#include "AppSettings.h"

#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <vector>
#include <array>

// Constants
constexpr uint32_t PBR_TEXTURE_START_BIND_INDEX = 2;
constexpr size_t PBR_MESH_TEXTURE_COUNT = 6;
constexpr size_t PBR_ENV_TEXTURE_COUNT = 3;

RendererPBR::RendererPBR(
	VulkanDevice& vkDev,
	std::vector<Model*> models,
	VulkanImage* envMap,
	VulkanImage* diffuseMap,
	VulkanImage* brdfLUT,
	VulkanImage* depthImage,
	VulkanImage* offscreenColorImage,
	uint8_t renderBit) :
	RendererBase(vkDev, depthImage, offscreenColorImage, renderBit),
	envMap_(envMap),
	diffuseMap_(diffuseMap),
	brdfLUT_(brdfLUT),
	models_(models)
{
	VkSampleCountFlagBits multisampleCount = VK_SAMPLE_COUNT_1_BIT;

	// Per frame UBO
	CreateUniformBuffers(vkDev, perFrameUBOs_, sizeof(PerFrameUBO));
	
	// Model UBO
	uint32_t numMeshes = 0u;
	for (Model* model : models_)
	{
		numMeshes += model->NumMeshes();
		CreateUniformBuffers(vkDev, model->modelBuffers_, sizeof(ModelUBO));
	}

	if (IsOffScreen())
	{
		multisampleCount = offscreenColorImage_->multisampleCount_;
		renderPass_.CreateOffScreenRenderPass(vkDev, renderBit, multisampleCount);
		CreateSingleFramebuffer(
			vkDev,
			renderPass_,
			{ 
				offscreenColorImage_->imageView_,
				depthImage_->imageView_ 
			},
			offscreenFramebuffer_);
	}
	else
	{
		// TODO Currently no MSAA for onscreen rendering
		renderPass_.CreateOnScreenRenderPass(vkDev, multisampleCount);
		CreateSwapchainFramebuffers(vkDev, renderPass_, depthImage_->imageView_);
	}

	CreateDescriptorPool(
		vkDev, 
		2 * models_.size(),  // (PerFrameUBO + ModelUBO) * modelSize
		0,  // SSBO
		(PBR_MESH_TEXTURE_COUNT + PBR_ENV_TEXTURE_COUNT) * numMeshes,
		numMeshes, // decsriptor count per swapchain
		&descriptorPool_);
	CreateDescriptorLayout(vkDev);

	for (Model* model : models_)
	{
		for (Mesh& mesh : model->meshes_)
		{
			CreateDescriptorSet(vkDev, model, mesh);
		}
	}

	CreatePipelineLayout(vkDev.GetDevice(), descriptorSetLayout_, &pipelineLayout_);

	CreateGraphicsPipeline(
		vkDev,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppSettings::ShaderFolder + "Mesh.vert",
			AppSettings::ShaderFolder + "Mesh.frag"
		},
		&graphicsPipeline_,
		true, // hasVertexBuffer
		multisampleCount // for multisampling
	);
}

RendererPBR::~RendererPBR()
{
}

void RendererPBR::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t swapchainImageIndex)
{
	renderPass_.BeginRenderPass(
		commandBuffer, 
		IsOffScreen() ? 
			offscreenFramebuffer_ : 
			swapchainFramebuffers_[swapchainImageIndex]);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);

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
				&mesh.descriptorSets_[swapchainImageIndex],
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

void RendererPBR::CreateDescriptorLayout(VulkanDevice& vkDev)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	uint32_t bindingIndex = 0;
	bindings.emplace_back(
		DescriptorSetLayoutBinding(
			bindingIndex++, 
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT));
	bindings.emplace_back(
		DescriptorSetLayoutBinding(
			bindingIndex++, 
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT));

	// PBR textures
	for (size_t i = 0; i < PBR_MESH_TEXTURE_COUNT; ++i)
	{
		bindings.emplace_back(
			DescriptorSetLayoutBinding(
				bindingIndex++,
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				VK_SHADER_STAGE_FRAGMENT_BIT)
		);
	}

	// envMap
	bindings.emplace_back(
		DescriptorSetLayoutBinding(
			bindingIndex++,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT)
	);

	// Diffuse map
	bindings.emplace_back(
		DescriptorSetLayoutBinding(
			bindingIndex++,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT)
	);

	// brdfLUT
	bindings.emplace_back(
		DescriptorSetLayoutBinding(
			bindingIndex++,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT)
	);

	const VkDescriptorSetLayoutCreateInfo layoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data()
	};

	VK_CHECK(vkCreateDescriptorSetLayout(vkDev.GetDevice(), &layoutInfo, nullptr, &descriptorSetLayout_));
}

void RendererPBR::CreateDescriptorSet(VulkanDevice& vkDev, Model* parentModel, Mesh& mesh)
{
	size_t swapchainLength = vkDev.GetSwapchainImageCount();

	std::vector<VkDescriptorSetLayout> layouts(swapchainLength, descriptorSetLayout_);

	const VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorPool_,
		.descriptorSetCount = static_cast<uint32_t>(swapchainLength),
		.pSetLayouts = layouts.data()
	};

	mesh.descriptorSets_.resize(swapchainLength);

	VK_CHECK(vkAllocateDescriptorSets(vkDev.GetDevice(), &allocInfo, mesh.descriptorSets_.data()));

	for (size_t i = 0; i < swapchainLength; i++)
	{
		VkDescriptorSet ds = mesh.descriptorSets_[i];

		const VkDescriptorBufferInfo bufferInfo1 = { perFrameUBOs_[i].buffer_, 0, sizeof(PerFrameUBO)};
		const VkDescriptorBufferInfo bufferInfo2 = { parentModel->modelBuffers_[i].buffer_, 0, sizeof(ModelUBO) };

		uint32_t bindIndex = 0;

		std::vector<VkWriteDescriptorSet> descriptorWrites;
		descriptorWrites.emplace_back(
			BufferWriteDescriptorSet(
				ds, 
				&bufferInfo1, 
				bindIndex++, 
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));
		descriptorWrites.emplace_back(
			BufferWriteDescriptorSet(
				ds, 
				&bufferInfo2, 
				bindIndex++, 
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));
		
		std::vector<VkDescriptorImageInfo> imageInfoArray;
		std::vector<uint32_t> bindIndexArray;
		for (const auto& elem : mesh.textures_)
		{
			imageInfoArray.emplace_back<VkDescriptorImageInfo>
				({
					elem.second->defaultImageSampler_,
					elem.second->imageView_,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
					});
			// The enum index starts from 1
			uint32_t meshBindIndex = PBR_TEXTURE_START_BIND_INDEX + static_cast<uint32_t>(elem.first) - 1;
			bindIndexArray.emplace_back(meshBindIndex);
		}

		for (size_t i = 0; i < imageInfoArray.size(); ++i)
		{
			descriptorWrites.emplace_back
			(
				ImageWriteDescriptorSet(
					ds, 
					&imageInfoArray[i],
					// Note that we don't use bindIndex
					bindIndexArray[i])
			);

			// Keep incrementing
			bindIndex++;
		}

		const VkDescriptorImageInfo imageInfoEnv = 
		{ 
			envMap_->defaultImageSampler_,
			envMap_->imageView_, 
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL 
		};
		const VkDescriptorImageInfo imageInfoEnvIrr = 
		{ 
			diffuseMap_->defaultImageSampler_,
			diffuseMap_->imageView_, 
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL 
		};
		const VkDescriptorImageInfo imageInfoBRDF = 
		{ 
			brdfLUT_->defaultImageSampler_,
			brdfLUT_->imageView_, 
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL 
		};

		descriptorWrites.emplace_back(
			ImageWriteDescriptorSet(ds, &imageInfoEnv, bindIndex++)
		);
		descriptorWrites.emplace_back(
			ImageWriteDescriptorSet(ds, &imageInfoEnvIrr, bindIndex++)
		);
		descriptorWrites.emplace_back(
			ImageWriteDescriptorSet(ds, &imageInfoBRDF, bindIndex++)
		);

		vkUpdateDescriptorSets
		(
			vkDev.GetDevice(), 
			static_cast<uint32_t>(descriptorWrites.size()), 
			descriptorWrites.data(), 
			0, 
			nullptr);
	}
}