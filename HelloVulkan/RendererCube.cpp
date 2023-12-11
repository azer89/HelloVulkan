#include "RendererCube.h"
#include "VulkanUtility.h"
#include "AppSettings.h"

#include "glm/glm.hpp"
#include "glm/ext.hpp"

// TODO weird STB errors
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_resize2.h"

#include <cmath>
#include <array>

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::ivec2;

template <typename T>
T Clamp(T v, T a, T b)
{
	if (v < a) return a;
	if (v > b) return b;
	return v;
}

static constexpr float PI = 3.14159265359f;
static constexpr float TWOPI = 6.28318530718f;

/// From Henry J. Warren's "Hacker's Delight"
float RadicalInverse_VdC(uint32_t bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10f; // / 0x100000000
}

// The i-th point is then computed by

/// From http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html

vec2 Hammersley2d(uint32_t i, uint32_t N)
{
	return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

inline VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t descriptorCount = 1)
{
	return VkDescriptorSetLayoutBinding{
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
	return VkWriteDescriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr,
		ds, bindIdx, 0, 1, dType, nullptr, bi, nullptr
	};
}

inline VkWriteDescriptorSet ImageWriteDescriptorSet(
	VkDescriptorSet ds, 
	const VkDescriptorImageInfo* ii, 
	uint32_t bindIdx)
{
	return VkWriteDescriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr,
		ds, bindIdx, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		ii, nullptr, nullptr
	};
}

RendererCube::RendererCube(VulkanDevice& vkDev, VulkanImage inDepthTexture, const char* textureFile) :
	RendererBase(vkDev, inDepthTexture)
{
	// Resource loading
	CreateCubeTextureImage(vkDev, textureFile, texture.image, texture.imageMemory);

	CreateImageView(vkDev.GetDevice(), texture.image, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, &texture.imageView, VK_IMAGE_VIEW_TYPE_CUBE, 6);
	CreateTextureSampler(vkDev.GetDevice(), &textureSampler);

	std::string vertexShader = AppSettings::ShaderFolder + "cube.vert";
	std::string fragmentShader = AppSettings::ShaderFolder + "cube.frag";

	// Pipeline initialization
	if (!CreateColorAndDepthRenderPass(vkDev, true, &renderPass_, RenderPassCreateInfo()) ||
		!CreateUniformBuffers(vkDev, sizeof(glm::mat4)) ||
		!CreateColorAndDepthFramebuffers(vkDev, renderPass_, depthTexture_.imageView, swapchainFramebuffers_) ||
		!CreateDescriptorPool(vkDev, 1, 0, 1, &descriptorPool_) ||
		!CreateDescriptorSet(vkDev) ||
		!CreatePipelineLayout(vkDev.GetDevice(), descriptorSetLayout_, &pipelineLayout_) ||
		!CreateGraphicsPipeline(vkDev, 
			renderPass_, 
			pipelineLayout_, 
			{ 
				vertexShader.c_str(),
				fragmentShader.c_str(),
			}, 
			&graphicsPipeline_))
	{
		printf("CubeRenderer: failed to create pipeline\n");
		exit(EXIT_FAILURE);
	}
}

RendererCube::~RendererCube()
{
	vkDestroySampler(device_, textureSampler, nullptr);
	texture.Destroy(device_);
}

void RendererCube::FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage)
{
	BeginRenderPass(commandBuffer, currentImage);

	vkCmdDraw(commandBuffer, 36, 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer);
}

void RendererCube::UpdateUniformBuffer(VulkanDevice& vkDev, uint32_t currentImage, const glm::mat4& m)
{
	UploadBufferData(vkDev, uniformBuffersMemory_[currentImage], 0, glm::value_ptr(m), sizeof(glm::mat4));
}

bool RendererCube::CreateDescriptorSet(VulkanDevice& vkDev)
{
	const std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
		DescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
		DescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
	};

	const VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data()
	};

	VK_CHECK(vkCreateDescriptorSetLayout(vkDev.GetDevice(), &layoutInfo, nullptr, &descriptorSetLayout_));

	auto swapChainImageSize = vkDev.GetSwapChainImageSize();

	std::vector<VkDescriptorSetLayout> layouts(swapChainImageSize, descriptorSetLayout_);

	const VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorPool_,
		.descriptorSetCount = static_cast<uint32_t>(swapChainImageSize),
		.pSetLayouts = layouts.data()
	};

	descriptorSets_.resize(swapChainImageSize);

	VK_CHECK(vkAllocateDescriptorSets(vkDev.GetDevice(), &allocInfo, descriptorSets_.data()));

	for (size_t i = 0; i < swapChainImageSize; i++)
	{
		VkDescriptorSet ds = descriptorSets_[i];

		const VkDescriptorBufferInfo bufferInfo = { uniformBuffers_[i], 0, sizeof(glm::mat4) };
		const VkDescriptorImageInfo  imageInfo = { textureSampler, texture.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

		const std::array<VkWriteDescriptorSet, 2> descriptorWrites = {
			BufferWriteDescriptorSet(ds, &bufferInfo,  0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
			ImageWriteDescriptorSet(ds, &imageInfo,   1)
		};

		vkUpdateDescriptorSets(vkDev.GetDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	return true;
}

bool RendererCube::CreateCubeTextureImage(
	VulkanDevice& vkDev, const char* filename, 
	VkImage& textureImage, 
	VkDeviceMemory& textureImageMemory, 
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

	return CreateTextureImageFromData(
		vkDev, 
		textureImage, 
		textureImageMemory,
		cube.data_.data(), 
		cube.w_, 
		cube.h_,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
}

Bitmap RendererCube::ConvertEquirectangularMapToVerticalCross(const Bitmap& b)
{
	if (b.type_ != eBitmapType_2D) return Bitmap();

	const int faceSize = b.w_ / 4;

	const int w = faceSize * 3;
	const int h = faceSize * 4;

	Bitmap result(w, h, b.comp_, b.fmt_);

	const ivec2 kFaceOffsets[] =
	{
		ivec2(faceSize, faceSize * 3),
		ivec2(0, faceSize),
		ivec2(faceSize, faceSize),
		ivec2(faceSize * 2, faceSize),
		ivec2(faceSize, 0),
		ivec2(faceSize, faceSize * 2)
	};

	const int clampW = b.w_ - 1;
	const int clampH = b.h_ - 1;

	for (int face = 0; face != 6; face++)
	{
		for (int i = 0; i != faceSize; i++)
		{
			for (int j = 0; j != faceSize; j++)
			{
				const vec3 P = FaceCoordsToXYZ(i, j, face, faceSize);
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
				const vec4 A = b.GetPixel(U1, V1);
				const vec4 B = b.GetPixel(U2, V1);
				const vec4 C = b.GetPixel(U1, V2);
				const vec4 D = b.GetPixel(U2, V2);
				// bilinear interpolation
				const vec4 color = A * (1 - s) * (1 - t) + B * (s) * (1 - t) + C * (1 - s) * t + D * (s) * (t);
				result.SetPixel(i + kFaceOffsets[face].x, j + kFaceOffsets[face].y, color);
			}
		};
	}

	return result;
}

Bitmap RendererCube::ConvertVerticalCrossToCubeMapFaces(const Bitmap& b)
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

inline Bitmap RendererCube::ConvertEquirectangularMapToCubeMapFaces(const Bitmap& b)
{
	return ConvertVerticalCrossToCubeMapFaces(ConvertEquirectangularMapToVerticalCross(b));
}

void RendererCube::ConvolveDiffuse(
	const glm::vec3* data,
	int srcW,
	int srcH,
	int dstW,
	int dstH,
	glm::vec3* output,
	int numMonteCarloSamples)
{
	// only equirectangular maps are supported
	assert(srcW == 2 * srcH);

	if (srcW != 2 * srcH) return;

	std::vector<vec3> tmp(dstW * dstH);

	stbir_resize(reinterpret_cast<const float*>(data),
		srcW,
		srcH,
		0,
		reinterpret_cast<float*>(tmp.data()),
		dstW,
		dstH,
		0,
		STBIR_RGB,
		STBIR_TYPE_FLOAT,
		STBIR_EDGE_CLAMP,
		STBIR_FILTER_CUBICBSPLINE);

	const vec3* scratch = tmp.data();
	srcW = dstW;
	srcH = dstH;

	for (int y = 0; y != dstH; y++)
	{
		printf("Line %i...\n", y);
		const float theta1 = float(y) / float(dstH) * PI;
		for (int x = 0; x != dstW; x++)
		{
			const float phi1 = float(x) / float(dstW) * TWOPI;
			const vec3 V1 = vec3(sin(theta1) * cos(phi1), sin(theta1) * sin(phi1), cos(theta1));
			vec3 color = vec3(0.0f);
			float weight = 0.0f;
			for (int i = 0; i != numMonteCarloSamples; i++)
			{
				const vec2 h = Hammersley2d(i, numMonteCarloSamples);
				const int x1 = int(floor(h.x * srcW));
				const int y1 = int(floor(h.y * srcH));
				const float theta2 = float(y1) / float(srcH) * PI;
				const float phi2 = float(x1) / float(srcW) * TWOPI;
				const vec3 V2 = vec3(sin(theta2) * cos(phi2), sin(theta2) * sin(phi2), cos(theta2));
				const float D = std::max(0.0f, glm::dot(V1, V2));
				if (D > 0.01f)
				{
					color += scratch[y1 * srcW + x1] * D;
					weight += D;
				}
			}
			output[y * dstW + x] = color / weight;
		}
	}
}

vec3 RendererCube::FaceCoordsToXYZ(int i, int j, int faceID, int faceSize)
{
	const float A = 2.0f * float(i) / faceSize;
	const float B = 2.0f * float(j) / faceSize;

	if (faceID == 0) return vec3(-1.0f, A - 1.0f, B - 1.0f);
	if (faceID == 1) return vec3(A - 1.0f, -1.0f, 1.0f - B);
	if (faceID == 2) return vec3(1.0f, A - 1.0f, 1.0f - B);
	if (faceID == 3) return vec3(1.0f - A, 1.0f, 1.0f - B);
	if (faceID == 4) return vec3(B - 1.0f, A - 1.0f, 1.0f);
	if (faceID == 5) return vec3(1.0f - B, A - 1.0f, -1.0f);

	return vec3();
}