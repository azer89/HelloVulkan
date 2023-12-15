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

RendererPBR::RendererPBR(
	VulkanDevice& vkDev,
	uint32_t uniformBufferSize,
	const char* modelFile,
	const char* texAOFile,
	const char* texEmissiveFile,
	const char* texAlbedoFile,
	const char* texMeRFile,
	const char* texNormalFile,
	const char* texEnvMapFile,
	const char* texIrrMapFile,
	VulkanImage depthTexture) : 
	RendererBase(vkDev, VulkanImage())
{
	depthTexture_ = depthTexture;

	// Resource loading part
	if (!CreateVertexBuffer(
		vkDev, 
		modelFile, 
		&storageBuffer_, 
		&vertexBufferSize_, 
		&indexBufferSize_))
	{
		printf("ModelRenderer: createPBRVertexBuffer() failed\n");
		exit(EXIT_FAILURE);
	}

	LoadTexture(vkDev, texAOFile, texAO_);
	LoadTexture(vkDev, texEmissiveFile, texEmissive_);
	LoadTexture(vkDev, texAlbedoFile, texAlbedo_);
	LoadTexture(vkDev, texMeRFile, texMeR_);
	LoadTexture(vkDev, texNormalFile, texNormal_);

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
		printf("ModelRenderer: failed to load BRDF LUT texture \n");
		exit(EXIT_FAILURE);
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

	std::string vertFile = AppSettings::ShaderFolder + "pbr_mesh.vert";
	std::string fragFile = AppSettings::ShaderFolder + "pbr_mesh.frag";

	if (!CreateColorAndDepthRenderPass(vkDev, true, &renderPass_, RenderPassCreateInfo()) ||
		!CreateUniformBuffers(vkDev, uniformBufferSize) ||
		!CreateColorAndDepthFramebuffers(vkDev, renderPass_, depthTexture_.imageView, swapchainFramebuffers_) ||
		!CreateDescriptorPool(vkDev, 1, 2, 8, &descriptorPool_) ||
		!CreateDescriptorSet(vkDev, uniformBufferSize) ||
		!CreatePipelineLayout(vkDev.GetDevice(), descriptorSetLayout_, &pipelineLayout_) ||
		!CreateGraphicsPipeline(
			vkDev, 
			renderPass_, 
			pipelineLayout_,
			{ 
				vertFile.c_str(),
				fragFile.c_str()
			},
			&graphicsPipeline_))
	{
		printf("PBRModelRenderer: failed to create pipeline\n");
		exit(EXIT_FAILURE);
	}
}

RendererPBR::~RendererPBR()
{
	storageBuffer_.Destroy(device_);

	texAO_.DestroyVulkanTexture(device_);
	texEmissive_.DestroyVulkanTexture(device_);
	texAlbedo_.DestroyVulkanTexture(device_);
	texMeR_.DestroyVulkanTexture(device_);
	texNormal_.DestroyVulkanTexture(device_);

	envMap_.DestroyVulkanTexture(device_);
	envMapIrradiance_.DestroyVulkanTexture(device_);

	brdfLUT_.DestroyVulkanTexture(device_);
}

void RendererPBR::FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage)
{
	BeginRenderPass(commandBuffer, currentImage);

	vkCmdDraw(commandBuffer, static_cast<uint32_t>(indexBufferSize_ / (sizeof(unsigned int))), 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);
}

void RendererPBR::UpdateUniformBuffer(VulkanDevice& vkDev, uint32_t currentImage, const void* data, const size_t dataSize)
{
	UploadBufferData(vkDev, uniformBuffers_[currentImage].bufferMemory_, 0, data, dataSize);
}

bool RendererPBR::CreateDescriptorSet(VulkanDevice& vkDev, uint32_t uniformDataSize)
{
	const std::vector<VkDescriptorSetLayoutBinding> bindings = {
		DescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT),
		DescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
		DescriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),

		DescriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),  // AO
		DescriptorSetLayoutBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),  // Emissive
		DescriptorSetLayoutBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),  // Albedo
		DescriptorSetLayoutBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),  // MeR
		DescriptorSetLayoutBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),  // Normal

		DescriptorSetLayoutBinding(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),  // env
		DescriptorSetLayoutBinding(9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),  // env_IRR

		DescriptorSetLayoutBinding(10, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)  // brdfLUT
	};

	const VkDescriptorSetLayoutCreateInfo layoutInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data()
	};

	VK_CHECK(vkCreateDescriptorSetLayout(vkDev.GetDevice(), &layoutInfo, nullptr, &descriptorSetLayout_));

	size_t swapchainLength = vkDev.GetSwapChainImageSize();

	std::vector<VkDescriptorSetLayout> layouts(swapchainLength, descriptorSetLayout_);

	const VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorPool_,
		.descriptorSetCount = static_cast<uint32_t>(swapchainLength),
		.pSetLayouts = layouts.data()
	};

	descriptorSets_.resize(swapchainLength);

	VK_CHECK(vkAllocateDescriptorSets(vkDev.GetDevice(), &allocInfo, descriptorSets_.data()));

	for (size_t i = 0; i < swapchainLength; i++)
	{
		VkDescriptorSet ds = descriptorSets_[i];

		const VkDescriptorBufferInfo bufferInfo = { uniformBuffers_[i].buffer_, 0, uniformDataSize };
		const VkDescriptorBufferInfo bufferInfo2 = { storageBuffer_.buffer_, 0, vertexBufferSize_ };
		const VkDescriptorBufferInfo bufferInfo3 = { storageBuffer_.buffer_, vertexBufferSize_, indexBufferSize_ };
		const VkDescriptorImageInfo  imageInfoAO = { texAO_.sampler, texAO_.image.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		const VkDescriptorImageInfo  imageInfoEmissive = { texEmissive_.sampler, texEmissive_.image.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		const VkDescriptorImageInfo  imageInfoAlbedo = { texAlbedo_.sampler, texAlbedo_.image.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		const VkDescriptorImageInfo  imageInfoMeR = { texMeR_.sampler, texMeR_.image.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		const VkDescriptorImageInfo  imageInfoNormal = { texNormal_.sampler, texNormal_.image.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

		const VkDescriptorImageInfo  imageInfoEnv = { envMap_.sampler, envMap_.image.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		const VkDescriptorImageInfo  imageInfoEnvIrr = { envMapIrradiance_.sampler, envMapIrradiance_.image.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		const VkDescriptorImageInfo  imageInfoBRDF = { brdfLUT_.sampler, brdfLUT_.image.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

		const std::array<VkWriteDescriptorSet, 11> descriptorWrites = {
			BufferWriteDescriptorSet(ds, &bufferInfo,  0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
			BufferWriteDescriptorSet(ds, &bufferInfo2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			BufferWriteDescriptorSet(ds, &bufferInfo3, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			ImageWriteDescriptorSet(ds, &imageInfoAO,       3),
			ImageWriteDescriptorSet(ds, &imageInfoEmissive, 4),
			ImageWriteDescriptorSet(ds, &imageInfoAlbedo,   5),
			ImageWriteDescriptorSet(ds, &imageInfoMeR,      6),
			ImageWriteDescriptorSet(ds, &imageInfoNormal,   7),

			ImageWriteDescriptorSet(ds, &imageInfoEnv,      8),
			ImageWriteDescriptorSet(ds, &imageInfoEnvIrr,   9),
			ImageWriteDescriptorSet(ds, &imageInfoBRDF,     10)
		};

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

bool RendererPBR::CreateVertexBuffer(
	VulkanDevice& vkDev,
	const char* filename,
	VulkanBuffer* storageBuffer,
	size_t* vertexBufferSize,
	size_t* indexBufferSize)
{
	const aiScene* scene = aiImportFile(filename, aiProcess_Triangulate);

	if (!scene || !scene->HasMeshes())
	{
		printf("Unable to load %s\n", filename);
		exit(255);
	}

	const aiMesh* mesh = scene->mMeshes[0];
	struct VertexData
	{
		glm::vec4 pos;
		glm::vec4 n;
		glm::vec4 tc;
	};

	std::vector<VertexData> vertices;
	for (unsigned i = 0; i != mesh->mNumVertices; i++)
	{
		const aiVector3D v = mesh->mVertices[i];
		const aiVector3D t = mesh->mTextureCoords[0][i];
		const aiVector3D n = mesh->mNormals[i];
		vertices.push_back(
			{ 
				.pos = glm::vec4(v.x, v.y, v.z, 1.0f), 
				.n = glm::vec4(n.x, n.y, n.z, 0.0f),
				.tc = glm::vec4(t.x, 1.0f - t.y, 0.0f, 0.0f) });
	}

	std::vector<unsigned int> indices;
	for (unsigned i = 0; i != mesh->mNumFaces; i++)
	{
		for (unsigned j = 0; j != 3; j++)
			indices.push_back(mesh->mFaces[i].mIndices[j]);
	}
	aiReleaseImport(scene);

	*vertexBufferSize = sizeof(VertexData) * vertices.size();
	*indexBufferSize = sizeof(unsigned int) * indices.size();

	AllocateVertexBuffer(vkDev, storageBuffer, *vertexBufferSize, vertices.data(), *indexBufferSize, indices.data());

	return true;
}

void RendererPBR::LoadTexture(VulkanDevice& vkDev, const char* fileName, VulkanTexture& texture)
{
	texture.CreateTextureImage(vkDev, fileName);

	texture.image.CreateImageView(
		vkDev.GetDevice(), 
		VK_FORMAT_R8G8B8A8_UNORM, 
		VK_IMAGE_ASPECT_COLOR_BIT);

	texture.CreateTextureSampler(vkDev.GetDevice());
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