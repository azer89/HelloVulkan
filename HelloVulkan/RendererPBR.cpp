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
	VulkanImage* depthImage,
	VulkanTexture* envMap,
	VulkanTexture* irradianceMap,
	const std::vector<MeshCreateInfo>& meshInfos) :
	RendererBase(vkDev, depthImage),
	envMap_(envMap),
	irradianceMap_(irradianceMap)
{
	for (const MeshCreateInfo& info : meshInfos)
	{
		LoadMesh(vkDev, info);
	}

	std::string brdfLUTFile = AppSettings::TextureFolder + "brdfLUT.ktx";

	gli::texture gliTex = gli::load_ktx(brdfLUTFile.c_str());
	glm::tvec3<GLsizei> extent(gliTex.extent(0));

	if (!brdfLUT_.image_.CreateImageFromData(
		vkDev,
		(uint8_t*)gliTex.data(0, 0, 0), 
		extent.x, 
		extent.y, 
		1, // mipmapCount
		1, // layerCount
		VK_FORMAT_R16G16_SFLOAT))
	{
		std::cerr << "ModelRenderer: failed to load BRDF LUT texture \n";;
	}

	brdfLUT_.image_.CreateImageView(
		vkDev.GetDevice(),
		VK_FORMAT_R16G16_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT
	);

	brdfLUT_.CreateTextureSampler(
		vkDev.GetDevice(),
		0.f,
		0.f,
		VK_FILTER_LINEAR,
		VK_FILTER_LINEAR,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
	);

	CreateColorAndDepthRenderPass(vkDev, true, &renderPass_, RenderPassCreateInfo());

	CreateUniformBuffers(vkDev, perFrameUBOs_, sizeof(PerFrameUBO));
	for (Mesh& mesh : meshes_)
	{
		CreateUniformBuffers(vkDev, mesh.modelBuffers_, sizeof(ModelUBO));
	}

	CreateColorAndDepthFramebuffers(vkDev, renderPass_, depthImage_->imageView_, swapchainFramebuffers_);

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

	CreateGraphicsPipeline(
		vkDev,
		renderPass_,
		pipelineLayout_,
		{
			AppSettings::ShaderFolder + "pbr_mesh.vert",
			AppSettings::ShaderFolder + "pbr_mesh.frag"
		},
		&graphicsPipeline_,
		true // hasVertexBuffer
	);
}

RendererPBR::~RendererPBR()
{
	for (Mesh& mesh : meshes_)
	{
		mesh.Destroy(device_);
	}

	//envMapIrradiance_.Destroy(device_);

	brdfLUT_.Destroy(device_);
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

bool RendererPBR::CreateDescriptorLayout(VulkanDevice& vkDev)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	uint32_t bindingIndex = 0;
	bindings.emplace_back(DescriptorSetLayoutBinding(bindingIndex++, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT));
	bindings.emplace_back(DescriptorSetLayoutBinding(bindingIndex++, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT));

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

	// irradianceMap
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

		const VkDescriptorBufferInfo bufferInfo1 = { perFrameUBOs_[i].buffer_, 0, sizeof(PerFrameUBO)};
		const VkDescriptorBufferInfo bufferInfo2 = { mesh.modelBuffers_[i].buffer_, 0, sizeof(ModelUBO) };

		uint32_t bindIndex = 0;

		std::vector<VkWriteDescriptorSet> descriptorWrites;
		descriptorWrites.emplace_back(
			BufferWriteDescriptorSet(ds, &bufferInfo1, bindIndex++, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));
		descriptorWrites.emplace_back(
			BufferWriteDescriptorSet(ds, &bufferInfo2, bindIndex++, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));
		
		std::vector<VkDescriptorImageInfo> imageInfos;
		std::vector<uint32_t> bindIndices;
		for (VulkanTexture& tex : mesh.textures_)
		{
			imageInfos.emplace_back<VkDescriptorImageInfo>
			({
				tex.sampler_,
				tex.image_.imageView_,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			});
			bindIndices.emplace_back(tex.bindIndex_);
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

		const VkDescriptorImageInfo imageInfoEnv = 
		{ 
			envMap_->sampler_, 
			envMap_->image_.imageView_, 
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL 
		};
		const VkDescriptorImageInfo imageInfoEnvIrr = 
		{ 
			irradianceMap_->sampler_, 
			irradianceMap_->image_.imageView_, 
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL 
		};
		const VkDescriptorImageInfo imageInfoBRDF = 
		{ 
			brdfLUT_.sampler_, 
			brdfLUT_.image_.imageView_, 
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

	return true;
}

void RendererPBR::LoadCubeMap(VulkanDevice& vkDev, const char* fileName, VulkanTexture& cubemap)
{
	cubemap.CreateCubeTextureImage(vkDev, fileName);

	uint32_t mipLevels = 1;
	cubemap.image_.CreateImageView(
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