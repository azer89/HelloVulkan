#include "RendererPBR.h"
#include "VulkanUtility.h"
#include "Bitmap.h"
#include "AppSettings.h"

#include "gli/gli.hpp"
#include "gli/load_ktx.hpp"

#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/cimport.h"

#include "stb_image.h"
#include "stb_image_resize2.h"

#include <vector>
#include <array>

static constexpr float PI = 3.14159265359f;
static constexpr float TWOPI = 6.28318530718f;

template <typename T>
T Clamp(T v, T a, T b)
{
	if (v < a) return a;
	if (v > b) return b;
	return v;
}

glm::vec3 FaceCoordsToXYZ(int i, int j, int faceID, int faceSize)
{
	const float A = 2.0f * float(i) / faceSize;
	const float B = 2.0f * float(j) / faceSize;

	if (faceID == 0) return glm::vec3(-1.0f, A - 1.0f, B - 1.0f);
	if (faceID == 1) return glm::vec3(A - 1.0f, -1.0f, 1.0f - B);
	if (faceID == 2) return glm::vec3(1.0f, A - 1.0f, 1.0f - B);
	if (faceID == 3) return glm::vec3(1.0f - A, 1.0f, 1.0f - B);
	if (faceID == 4) return glm::vec3(B - 1.0f, A - 1.0f, 1.0f);
	if (faceID == 5) return glm::vec3(1.0f - B, A - 1.0f, -1.0f);

	return glm::vec3();
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
	if (!CreatePBRVertexBuffer(
		vkDev, 
		modelFile, 
		&storageBuffer_.buffer_, 
		&storageBuffer_.bufferMemory_, 
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
	UploadBufferData(vkDev, uniformBuffersMemory_[currentImage], 0, data, dataSize);
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

		const VkDescriptorBufferInfo bufferInfo = { uniformBuffers_[i], 0, uniformDataSize };
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

bool RendererPBR::CreatePBRVertexBuffer(
	VulkanDevice& vkDev,
	const char* filename,
	VkBuffer* storageBuffer,
	VkDeviceMemory* storageBufferMemory,
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

	AllocateVertexBuffer(vkDev, storageBuffer, storageBufferMemory, *vertexBufferSize, vertices.data(), *indexBufferSize, indices.data());

	return true;
}

void RendererPBR::LoadTexture(VulkanDevice& vkDev, const char* fileName, VulkanTexture& texture)
{
	CreateTextureImage(vkDev, fileName, texture);

	texture.image.CreateImageView(
		vkDev.GetDevice(), 
		VK_FORMAT_R8G8B8A8_UNORM, 
		VK_IMAGE_ASPECT_COLOR_BIT);

	texture.CreateTextureSampler(vkDev.GetDevice());
}

void RendererPBR::LoadCubeMap(VulkanDevice& vkDev, const char* fileName, VulkanTexture& cubemap)
{
	CreateCubeTextureImage(vkDev, fileName, cubemap);

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

bool RendererPBR::CreateTextureImage(
	VulkanDevice& vkDev,
	const char* filename,
	VulkanTexture& texture,
	uint32_t* outTexWidth,
	uint32_t* outTexHeight)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels)
	{
		printf("Failed to load [%s] texture\n", filename); fflush(stdout);
		return false;
	}

	bool result = texture.image.CreateTextureImageFromData(
		vkDev, 
		pixels, 
		texWidth, 
		texHeight, 
		VK_FORMAT_R8G8B8A8_UNORM);

	stbi_image_free(pixels);

	if (outTexWidth && outTexHeight)
	{
		*outTexWidth = (uint32_t)texWidth;
		*outTexHeight = (uint32_t)texHeight;
	}

	return result;
}

bool RendererPBR::CreateCubeTextureImage(
	VulkanDevice& vkDev,
	const char* filename,
	VulkanTexture& texture,
	uint32_t* width,
	uint32_t* height)
{
	int w, h, comp;
	const float* img = stbi_loadf(filename, &w, &h, &comp, 3);
	std::vector<float> img32(w * h * 4);

	Float24to32(w, h, img, img32.data());

	if (!img)
	{
		printf("Failed to load [%s] texture\n", filename); fflush(stdout);
		return false;
	}

	stbi_image_free((void*)img);

	Bitmap in(w, h, 4, eBitmapFormat_Float, img32.data());
	Bitmap out = ConvertEquirectangularMapToVerticalCross(in);

	Bitmap cube = ConvertVerticalCrossToCubeMapFaces(out);

	if (width && height)
	{
		*width = w;
		*height = h;
	}

	return texture.image.CreateTextureImageFromData(vkDev, 
		cube.data_.data(), 
		cube.w_, 
		cube.h_,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
}

Bitmap RendererPBR::ConvertEquirectangularMapToVerticalCross(const Bitmap& b)
{
	if (b.type_ != eBitmapType_2D) return Bitmap();

	const int faceSize = b.w_ / 4;

	const int w = faceSize * 3;
	const int h = faceSize * 4;

	Bitmap result(w, h, b.comp_, b.fmt_);

	const glm::ivec2 kFaceOffsets[] =
	{
		glm::ivec2(faceSize, faceSize * 3),
		glm::ivec2(0, faceSize),
		glm::ivec2(faceSize, faceSize),
		glm::ivec2(faceSize * 2, faceSize),
		glm::ivec2(faceSize, 0),
		glm::ivec2(faceSize, faceSize * 2)
	};

	const int clampW = b.w_ - 1;
	const int clampH = b.h_ - 1;

	for (int face = 0; face != 6; face++)
	{
		for (int i = 0; i != faceSize; i++)
		{
			for (int j = 0; j != faceSize; j++)
			{
				const glm::vec3 P = FaceCoordsToXYZ(i, j, face, faceSize);
				const float R = hypot(P.x, P.y);
				const float theta = atan2(P.y, P.x);
				const float phi = atan2(P.z, R);
				//	float point source coordinates
				const float Uf = float(2.0f * faceSize * (theta + PI) / PI);
				const float Vf = float(2.0f * faceSize * (PI / 2.0f - phi) / PI);
				// 4-samples for bilinear interpolation
				const int U1 = Clamp(int(floor(Uf)), 0, clampW);
				const int V1 = Clamp(int(floor(Vf)), 0, clampH);
				const int U2 = Clamp(U1 + 1, 0, clampW);
				const int V2 = Clamp(V1 + 1, 0, clampH);
				// fractional part
				const float s = Uf - U1;
				const float t = Vf - V1;
				// fetch 4-samples
				const glm::vec4 A = b.GetPixel(U1, V1);
				const glm::vec4 B = b.GetPixel(U2, V1);
				const glm::vec4 C = b.GetPixel(U1, V2);
				const glm::vec4 D = b.GetPixel(U2, V2);
				// bilinear interpolation
				const glm::vec4 color = A * (1 - s) * (1 - t) + B * (s) * (1 - t) + C * (1 - s) * t + D * (s) * (t);
				result.SetPixel(i + kFaceOffsets[face].x, j + kFaceOffsets[face].y, color);
			}
		};
	}

	return result;
}

Bitmap RendererPBR::ConvertVerticalCrossToCubeMapFaces(const Bitmap& b)
{
	const int faceWidth = b.w_ / 3;
	const int faceHeight = b.h_ / 4;

	Bitmap cubemap(faceWidth, faceHeight, 6, b.comp_, b.fmt_);
	cubemap.type_ = eBitmapType_Cube;

	const uint8_t* src = b.data_.data();
	uint8_t* dst = cubemap.data_.data();

	/*
			------
			| +Y |
	 ----------------
	 | -X | -Z | +X |
	 ----------------
			| -Y |
			------
			| +Z |
			------
	*/

	const int pixelSize = cubemap.comp_ * Bitmap::GetBytesPerComponent(cubemap.fmt_);

	for (int face = 0; face != 6; ++face)
	{
		for (int j = 0; j != faceHeight; ++j)
		{
			for (int i = 0; i != faceWidth; ++i)
			{
				int x = 0;
				int y = 0;

				switch (face)
				{
					// GL_TEXTURE_CUBE_MAP_POSITIVE_X
				case 0:
					x = i;
					y = faceHeight + j;
					break;

					// GL_TEXTURE_CUBE_MAP_NEGATIVE_X
				case 1:
					x = 2 * faceWidth + i;
					y = 1 * faceHeight + j;
					break;

					// GL_TEXTURE_CUBE_MAP_POSITIVE_Y
				case 2:
					x = 2 * faceWidth - (i + 1);
					y = 1 * faceHeight - (j + 1);
					break;

					// GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
				case 3:
					x = 2 * faceWidth - (i + 1);
					y = 3 * faceHeight - (j + 1);
					break;

					// GL_TEXTURE_CUBE_MAP_POSITIVE_Z
				case 4:
					x = 2 * faceWidth - (i + 1);
					y = b.h_ - (j + 1);
					break;

					// GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
				case 5:
					x = faceWidth + i;
					y = faceHeight + j;
					break;
				}

				memcpy(dst, src + (y * b.w_ + x) * pixelSize, pixelSize);

				dst += pixelSize;
			}
		}
	}

	return cubemap;
}
