#include "RendererPBR.h"
#include "VulkanUtility.h"
#include "AppSettings.h"

#include "gli/gli.hpp"
#include "gli/load_ktx.hpp"

#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/cimport.h"

#include <vector>
#include <array>

// Constants
constexpr uint32_t PBR_TEXTURE_START_BIND_INDEX = 2;
constexpr size_t PBR_MESH_TEXTURE_COUNT = 5;
constexpr size_t PBR_ENV_TEXTURE_COUNT = 3;

RendererPBR::RendererPBR(
	VulkanDevice& vkDev,
	/*const char* modelFile,
	const char* texAOFile,
	const char* texEmissiveFile,
	const char* texAlbedoFile,
	const char* texMeRFile,
	const char* texNormalFile,*/
	const std::vector<MeshCreateInfo>& meshInfos,
	const char* texEnvMapFile,
	const char* texIrrMapFile,
	VulkanImage depthTexture) : 
	RendererBase(vkDev, VulkanImage())
{
	depthTexture_ = depthTexture;

	for (const MeshCreateInfo& info : meshInfos)
	{
		LoadMesh(vkDev, info);
	}

	// cube maps
	LoadCubeMap(vkDev, texEnvMapFile, envMap_);
	LoadCubeMap(vkDev, texIrrMapFile, envMapIrradiance_);

	std::string brdfLUTFile = AppSettings::TextureFolder + "brdfLUT.ktx";

	gli::texture gliTex = gli::load_ktx(brdfLUTFile.c_str());
	glm::tvec3<GLsizei> extent(gliTex.extent(0));

	if (!brdfLUT_.image.CreateTextureImageFromData(
		vkDev,
		(uint8_t*)gliTex.data(0, 0, 0), 
		extent.x, 
		extent.y, 
		VK_FORMAT_R16G16_SFLOAT))
	{
		std::cerr << "ModelRenderer: failed to load BRDF LUT texture \n";;
	}

	brdfLUT_.image.CreateImageView(
		vkDev.GetDevice(),
		VK_FORMAT_R16G16_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT
	);

	brdfLUT_.CreateTextureSampler(
		vkDev.GetDevice(),
		VK_FILTER_LINEAR,
		VK_FILTER_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
	);

	CreateColorAndDepthRenderPass(vkDev, true, &renderPass_, RenderPassCreateInfo());

	CreateUniformBuffers(vkDev, uniformBuffers_, sizeof(PerFrameUBO));
	for (Mesh& mesh : meshes_)
	{
		CreateUniformBuffers(vkDev, mesh.modelBuffers_, sizeof(ModelUBO));
	}

	CreateColorAndDepthFramebuffers(vkDev, renderPass_, depthTexture_.imageView, swapchainFramebuffers_);

	CreateDescriptorPool(
		vkDev, 
		2 * meshes_.size(),  // (PerFrameUBO + ModelUBO) * meshes_.size()
		0,  // SSBO
		(PBR_MESH_TEXTURE_COUNT + PBR_ENV_TEXTURE_COUNT) * meshes_.size(),
		meshes_.size(), // decsriptor count per swapchain
		&descriptorPool_);
	CreateDescriptorLayout(vkDev);
	for (Mesh& mesh : meshes_)
	{
		CreateDescriptorSet(vkDev, mesh);
	}

	CreatePipelineLayout(vkDev.GetDevice(), descriptorSetLayout_, &pipelineLayout_);

	std::string vertFile = AppSettings::ShaderFolder + "pbr_mesh.vert";
	std::string fragFile = AppSettings::ShaderFolder + "pbr_mesh.frag";
	CreateGraphicsPipeline(
		vkDev,
		renderPass_,
		pipelineLayout_,
		{
			vertFile.c_str(),
			fragFile.c_str()
		},
		&graphicsPipeline_,
		true // hasVertexBuffer
	);
}

RendererPBR::~RendererPBR()
{
	//mesh_.Destroy(device_);
	for (Mesh& mesh : meshes_)
	{
		mesh.Destroy(device_);
	}

	envMap_.DestroyVulkanTexture(device_);
	envMapIrradiance_.DestroyVulkanTexture(device_);

	brdfLUT_.DestroyVulkanTexture(device_);
}

void RendererPBR::FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage)
{
	BeginRenderPass(commandBuffer, currentImage);

	for (Mesh& mesh : meshes_)
	{
		vkCmdBindDescriptorSets(
			commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout_,
			0,
			1,
			&mesh.descriptorSets_[currentImage],
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
	
	vkCmdEndRenderPass(commandBuffer);
}

inline VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(
	uint32_t binding,
	VkDescriptorType descriptorType,
	VkShaderStageFlags stageFlags,
	uint32_t descriptorCount = 1)
{
	return VkDescriptorSetLayoutBinding
	{
		.binding = binding,
		.descriptorType = descriptorType,
		.descriptorCount = descriptorCount,
		.stageFlags = stageFlags,
		.pImmutableSamplers = nullptr
	};
}

inline VkWriteDescriptorSet BufferWriteDescriptorSet(
	VkDescriptorSet ds,
	const VkDescriptorBufferInfo* bi,
	uint32_t bindIdx,
	VkDescriptorType dType)
{
	return VkWriteDescriptorSet
	{
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		nullptr,
		ds,
		bindIdx,
		0,
		1,
		dType,
		nullptr,
		bi,
		nullptr
	};
}

inline VkWriteDescriptorSet ImageWriteDescriptorSet(
	VkDescriptorSet ds,
	const VkDescriptorImageInfo* ii,
	uint32_t bindIdx)
{
	return VkWriteDescriptorSet
	{
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		nullptr,
		ds,
		bindIdx,
		0,
		1,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		ii,
		nullptr,
		nullptr
	};
}

bool RendererPBR::CreateDescriptorLayout(VulkanDevice& vkDev)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	uint32_t bindingIndex = 0;
	bindings.emplace_back(DescriptorSetLayoutBinding(bindingIndex++, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT));
	bindings.emplace_back(DescriptorSetLayoutBinding(bindingIndex++, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT));
	//bindings.emplace_back(DescriptorSetLayoutBinding(bindingIndex++, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT));
	//bindings.emplace_back(DescriptorSetLayoutBinding(bindingIndex++, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT));

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

	// env
	bindings.emplace_back(
		DescriptorSetLayoutBinding(
			bindingIndex++,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT)
	);

	// env_IRR
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

	return true;
}

bool RendererPBR::CreateDescriptorSet(VulkanDevice& vkDev, Mesh& mesh)
{
	size_t swapchainLength = vkDev.GetSwapChainImageSize();

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

		const VkDescriptorBufferInfo bufferInfo1 = { uniformBuffers_[i].buffer_, 0, sizeof(PerFrameUBO)};
		const VkDescriptorBufferInfo bufferInfo2 = { mesh.modelBuffers_[i].buffer_, 0, sizeof(ModelUBO) };

		uint32_t bindIndex = 0;

		std::vector<VkWriteDescriptorSet> descriptorWrites;
		descriptorWrites.emplace_back(BufferWriteDescriptorSet(ds, &bufferInfo1, bindIndex++, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));
		descriptorWrites.emplace_back(BufferWriteDescriptorSet(ds, &bufferInfo2, bindIndex++, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));
		
		std::vector<VkDescriptorImageInfo> imageInfos;
		std::vector<uint32_t> bindIndices;
		for (VulkanTexture& tex : mesh.textures_)
		{
			imageInfos.emplace_back<VkDescriptorImageInfo>
			({
				tex.sampler,
				tex.image.imageView,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			});
			bindIndices.emplace_back(tex.bindIndex);
		}

		for (size_t i = 0; i < imageInfos.size(); ++i)
		{
			descriptorWrites.emplace_back
			(
				ImageWriteDescriptorSet(
					ds, 
					&imageInfos[i], 
					// Note that we don't use bindIndex
					bindIndices[i])
			);

			// Keep incrementing
			bindIndex++;
		}

		const VkDescriptorImageInfo imageInfoEnv = { envMap_.sampler, envMap_.image.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		const VkDescriptorImageInfo imageInfoEnvIrr = { envMapIrradiance_.sampler, envMapIrradiance_.image.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		const VkDescriptorImageInfo imageInfoBRDF = { brdfLUT_.sampler, brdfLUT_.image.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

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

	return true;
}

void RendererPBR::LoadCubeMap(VulkanDevice& vkDev, const char* fileName, VulkanTexture& cubemap)
{
	cubemap.CreateCubeTextureImage(vkDev, fileName);

	uint32_t mipLevels = 1;
	cubemap.image.CreateImageView(
		vkDev.GetDevice(),
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_CUBE,
		6,
		mipLevels);

	cubemap.CreateTextureSampler(vkDev.GetDevice());
}

void RendererPBR::LoadMesh(VulkanDevice& vkDev, const MeshCreateInfo& info)
{
	Mesh mesh;
	mesh.Create(vkDev, info.modelFile.c_str());
	uint32_t bindIndex = PBR_TEXTURE_START_BIND_INDEX;
	for (const std::string& texFile : info.textureFiles)
	{
		mesh.AddTexture(vkDev, texFile.c_str(), bindIndex++);
	}
	meshes_.push_back(mesh);
}