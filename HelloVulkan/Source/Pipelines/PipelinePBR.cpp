#include "PipelinePBR.h"
#include "VulkanUtility.h"
#include "Configs.h"

#include <vector>
#include <array>

// Constants
constexpr uint32_t UBO_COUNT = 2;
constexpr uint32_t SSBO_COUNT = 1;
constexpr size_t PBR_MESH_TEXTURE_COUNT = 6; 
constexpr size_t PBR_ENV_TEXTURE_COUNT = 3; // Specular, diffuse, and BRDF LUT

PipelinePBR::PipelinePBR(
	VulkanDevice& vkDev,
	std::vector<Model*> models,
	Lights* lights,
	VulkanImage* specularMap,
	VulkanImage* diffuseMap,
	VulkanImage* brdfLUT,
	VulkanImage* depthImage,
	VulkanImage* offscreenColorImage,
	uint8_t renderBit) :
	PipelineBase(vkDev, PipelineFlags::GraphicsOffScreen), // Offscreen
	models_(models),
	lights_(lights),
	specularCubemap_(specularMap),
	diffuseCubemap_(diffuseMap),
	brdfLUT_(brdfLUT)
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

	// Note that this pipeline is offscreen rendering
	multisampleCount = offscreenColorImage->multisampleCount_;
	renderPass_.CreateOffScreenRenderPass(vkDev, renderBit, multisampleCount);

	framebuffer_.Create(
		vkDev, 
		renderPass_.GetHandle(), 
		{
			offscreenColorImage,
			depthImage
		}, 
		IsOffscreen());

	CreateDescriptorPool(
		vkDev, 
		UBO_COUNT * models_.size(), 
		SSBO_COUNT,
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

	// Push constants
	std::vector<VkPushConstantRange> ranges(1u);
	VkPushConstantRange& range = ranges.front();
	range.offset = 0u;
	range.size = sizeof(PushConstantPBR);
	range.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	CreatePipelineLayout(vkDev.GetDevice(), descriptorSetLayout_, &pipelineLayout_, ranges);

	CreateGraphicsPipeline(
		vkDev,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "Mesh.vert",
			AppConfig::ShaderFolder + "Mesh.frag"
		},
		&pipeline_,
		true, // hasVertexBuffer
		multisampleCount // for multisampling
	);
}

PipelinePBR::~PipelinePBR()
{
}

void PipelinePBR::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t swapchainImageIndex)
{
	renderPass_.BeginRenderPass(vkDev, commandBuffer, framebuffer_.GetFramebuffer());

	BindPipeline(vkDev, commandBuffer);

	vkCmdPushConstants(
		commandBuffer,
		pipelineLayout_,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(PushConstantPBR), &pc_);

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

void PipelinePBR::CreateDescriptorLayout(VulkanDevice& vkDev)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	uint32_t bindingIndex = 0;
	// UBO
	bindings.emplace_back(
		DescriptorSetLayoutBinding(
			bindingIndex++, 
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT));
	// UBO
	bindings.emplace_back(
		DescriptorSetLayoutBinding(
			bindingIndex++, 
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT));
	// SSBO
	bindings.emplace_back(
		DescriptorSetLayoutBinding(
			bindingIndex++,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
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

	// Specular map
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

void PipelinePBR::CreateDescriptorSet(VulkanDevice& vkDev, Model* parentModel, Mesh& mesh)
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

	// Mesh textures
	std::vector<VkDescriptorImageInfo> imageInfoArray;
	std::vector<uint32_t> bindIndexArray;
	for (const auto& elem : mesh.textures_)
	{
		imageInfoArray.push_back(elem.second->GetDescriptorImageInfo());
		// The enum index starts from 1
		uint32_t meshBindIndex = UBO_COUNT + SSBO_COUNT + static_cast<uint32_t>(elem.first) - 1;
		bindIndexArray.emplace_back(meshBindIndex);
	}

	// PBR textures
	const VkDescriptorImageInfo specularImageInfo = specularCubemap_->GetDescriptorImageInfo();
	const VkDescriptorImageInfo diffuseImageInfo = diffuseCubemap_->GetDescriptorImageInfo();
	const VkDescriptorImageInfo BRDFLUTImageInfo = brdfLUT_->GetDescriptorImageInfo();

	// Create a descriptor per swapchain
	for (size_t i = 0; i < swapchainLength; i++)
	{
		uint32_t bindIndex = 0;

		VkDescriptorSet ds = mesh.descriptorSets_[i];

		const VkDescriptorBufferInfo bufferInfo1 = { perFrameUBOs_[i].buffer_, 0, sizeof(PerFrameUBO)};
		const VkDescriptorBufferInfo bufferInfo2 = { parentModel->modelBuffers_[i].buffer_, 0, sizeof(ModelUBO) };
		const VkDescriptorBufferInfo bufferInfo3 = { lights_->GetSSBOBuffer(), 0, lights_->GetSSBOSize() };

		// UBO
		std::vector<VkWriteDescriptorSet> descriptorWrites;
		descriptorWrites.emplace_back(
			BufferWriteDescriptorSet(
				ds, 
				&bufferInfo1, 
				bindIndex++, 
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));
		// UBO
		descriptorWrites.emplace_back(
			BufferWriteDescriptorSet(
				ds, 
				&bufferInfo2, 
				bindIndex++, 
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));
		// SSBO
		descriptorWrites.emplace_back(
			BufferWriteDescriptorSet(
				ds,
				&bufferInfo3,
				bindIndex++,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER));

		// Mesh textures
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

		// PBR textures
		descriptorWrites.emplace_back(
			ImageWriteDescriptorSet(ds, &specularImageInfo, bindIndex++)
		);
		descriptorWrites.emplace_back(
			ImageWriteDescriptorSet(ds, &diffuseImageInfo, bindIndex++)
		);
		descriptorWrites.emplace_back(
			ImageWriteDescriptorSet(ds, &BRDFLUTImageInfo, bindIndex++)
		);

		vkUpdateDescriptorSets
		(
			vkDev.GetDevice(), 
			static_cast<uint32_t>(descriptorWrites.size()), 
			descriptorWrites.data(), 
			0, 
			nullptr
		);
	}
}