#include "VulkanTexture.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_resize2.h"

static constexpr float PI = 3.14159265359f;
static constexpr float TWOPI = 6.28318530718f;

template <typename T>
T Clamp(T v, T a, T b)
{
	if (v < a) return a;
	if (v > b) return b;
	return v;
}

inline int NumMipmap(int width, int height)
{
	return static_cast<int>(floor(log2(std::max(width, height)))) + 1;
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

void Float24to32(int w, int h, const float* img24, float* img32)
{
	const int numPixels = w * h;
	for (int i = 0; i != numPixels; i++)
	{
		*img32++ = *img24++;
		*img32++ = *img24++;
		*img32++ = *img24++;
		*img32++ = 1.0f;
	}
}

bool VulkanTexture::CreateTextureSampler(
	VkDevice device,
	VkFilter minFilter,
	VkFilter maxFilter,
	VkSamplerAddressMode addressMode)
{
	const VkSamplerCreateInfo samplerInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = addressMode, // VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = addressMode, // VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = addressMode, // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_FALSE,
		.maxAnisotropy = 1,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE
	};

	return (vkCreateSampler(device, &samplerInfo, nullptr, &sampler_) == VK_SUCCESS);
}

void VulkanTexture::CreateTexture(
	VulkanDevice& vkDev,
	uint32_t width,
	uint32_t height,
	uint32_t layers,
	VkFormat format,
	uint32_t levels,
	VkImageUsageFlags additionalUsage)
{
	width_ = width;
	height_ = height;
	layers_ = layers;
	mipmapLevels_ = (levels > 0) ? levels : NumMipmap(width, height);

	VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | additionalUsage;
	if (mipmapLevels_ > 1)
	{
		usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT; // For mipmap generation
	}

	//texture.image = createImage(width, height, layers, texture.levels, format, 1, usage);
	//texture.view = createTextureView(texture, format, VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS);*/
	image_.CreateImage(
		vkDev.GetDevice(),
		vkDev.GetPhysicalDevice(),
		width_,
		height_,
		format,
		VK_IMAGE_TILING_OPTIMAL,
		usage,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		(layers_ == 6) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0,
		mipmapLevels_);

	image_.CreateImageView(
		vkDev.GetDevice(),
		format,
		VK_IMAGE_ASPECT_COLOR_BIT,
		(layers_ == 6) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D,
		layers_,
		mipmapLevels_
	);
}

bool VulkanTexture::CreateHDRImage(
	VulkanDevice& vkDev,
	const char* filename)
{
	int texWidth, texHeight, texChannels;
	float* pixels = stbi_loadf(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	bool result = image_.CreateTextureImageFromData(
		vkDev,
		pixels,
		texWidth,
		texHeight,
		VK_FORMAT_R32G32B32A32_SFLOAT);

	stbi_image_free(pixels);

	return result;
}

bool VulkanTexture::CreateTextureImage(
	VulkanDevice& vkDev,
	const char* filename,
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

	bool result = image_.CreateTextureImageFromData(
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

bool VulkanTexture::CreateCubeTextureImage(
	VulkanDevice& vkDev,
	const char* filename,
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

	return image_.CreateTextureImageFromData(vkDev,
		cube.data_.data(),
		cube.w_,
		cube.h_,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
}

Bitmap VulkanTexture::ConvertEquirectangularMapToVerticalCross(const Bitmap& b)
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

Bitmap VulkanTexture::ConvertVerticalCrossToCubeMapFaces(const Bitmap& b)
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

void VulkanTexture::DestroyVulkanTexture(VkDevice device)
{
	DestroyVulkanImage(device);
	vkDestroySampler(device, sampler_, nullptr);
}

void VulkanTexture::DestroyVulkanImage(VkDevice device)
{
	vkDestroyImageView(device, image_.imageView, nullptr);
	vkDestroyImage(device, image_.image, nullptr);
	vkFreeMemory(device, image_.imageMemory, nullptr);
}