#include "ResourcesGBuffer.h"
#include "Utility.h"
#include "VulkanCheck.h"

constexpr uint32_t SSAO_NOISE_DIM = 16;
constexpr uint32_t SSAO_NOISE_LENGTH = SSAO_NOISE_DIM * SSAO_NOISE_DIM;
constexpr uint32_t SSAO_KERNEL_SIZE = 64;

ResourcesGBuffer::ResourcesGBuffer()
{
}

ResourcesGBuffer::~ResourcesGBuffer()
{
	Destroy();
}

void ResourcesGBuffer::Destroy()
{
	position_.Destroy();
	normal_.Destroy();
	noise_.Destroy();
	ssao_.Destroy();
	depth_.Destroy();
	kernel_.Destroy();
}

float ResourcesGBuffer::GetNoiseDimension() const
{
	return static_cast<float>(SSAO_NOISE_DIM);
}

void ResourcesGBuffer::OnWindowResized(VulkanContext& ctx)
{
	Create(ctx);
}

void ResourcesGBuffer::Create(VulkanContext& ctx)
{
	const uint32_t width = ctx.GetSwapchainWidth();
	const uint32_t height = ctx.GetSwapchainHeight();

	position_.Destroy();
	normal_.Destroy();
	depth_.Destroy();
	ssao_.Destroy();

	// Position buffer
	position_.CreateImage(
		ctx,
		width,
		height,
		1u, // mip
		1u, // layer
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		0u,
		VK_SAMPLE_COUNT_1_BIT);
	position_.CreateImageView(
		ctx,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_2D,
		0u,
		1u,
		0u,
		1u);
	CreateSampler(ctx, &(position_.defaultImageSampler_));
	position_.SetDebugName(ctx, "G_Buffer_Position");

	// Normal buffer
	normal_.CreateImage(
		ctx,
		width,
		height,
		1u, // mip
		1u, // layer
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		0u,
		VK_SAMPLE_COUNT_1_BIT);
	normal_.CreateImageView(
		ctx,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_2D,
		0u,
		1u,
		0u,
		1u);
	CreateSampler(ctx, &(normal_.defaultImageSampler_));
	normal_.SetDebugName(ctx, "G_Buffer_Normal");

	// Normal buffer
	ssao_.CreateImage(
		ctx,
		width,
		height,
		1u, // mip
		1u, // layer
		VK_FORMAT_R8_UNORM,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		0u,
		VK_SAMPLE_COUNT_1_BIT);
	ssao_.CreateImageView(
		ctx,
		VK_FORMAT_R8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_2D,
		0u,
		1u,
		0u,
		1u);
	CreateSampler(ctx, &(ssao_.defaultImageSampler_));
	ssao_.SetDebugName(ctx, "G_Buffer_SSAO");

	// Needed for depth test
	depth_.CreateDepthAttachment(
		ctx,
		width,
		height,
		1u, // layerCount
		VK_SAMPLE_COUNT_1_BIT);
	depth_.SetDebugName(ctx, "Depth_Image");

	// Noise texture
	if (noise_.image_ == nullptr)
	{
		std::vector<glm::vec2> ssaoNoise(SSAO_NOISE_LENGTH);
		for (uint32_t i = 0; i < SSAO_NOISE_LENGTH; ++i)
		{
			ssaoNoise[i] = glm::vec2(
				Utility::RandomNumber() * 2.0 - 1.0,
				Utility::RandomNumber() * 2.0 - 1.0);
		}
		noise_.CreateImageFromData(
			ctx,
			ssaoNoise.data(), // imageData
			SSAO_NOISE_DIM, // texWidth
			SSAO_NOISE_DIM, // texHeight
			1u, // mipmapCount
			1u, // layerCount
			VK_FORMAT_R32G32_SFLOAT);
		noise_.CreateImageView(
			ctx,
			VK_FORMAT_R32G32_SFLOAT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_VIEW_TYPE_2D,
			0u,
			1u,
			0u,
			1u);
		CreateSampler(ctx, &(noise_.defaultImageSampler_));
		noise_.SetDebugName(ctx, "G_Buffer_Noise");
	}

	// Kernel buffer
	if (kernel_.buffer_ == nullptr)
	{
		ssaoKernel_.resize(SSAO_KERNEL_SIZE);
		for (int i = 0; i < SSAO_KERNEL_SIZE; ++i)
		{
			glm::vec3 sample(Utility::RandomNumber(-1.0f, 1.0f),
				Utility::RandomNumber(-1.0f, 1.0f),
				Utility::RandomNumber()); // Half hemisphere
			sample = glm::normalize(sample);
			sample *= Utility::RandomNumber();
			float scale = static_cast<float>(i) / SSAO_KERNEL_SIZE;

			// Scale samples s.t. they're more aligned to center of kernel
			scale = Utility::Lerp(0.1f, 1.0f, scale * scale);
			sample *= scale;
			ssaoKernel_[i] = glm::vec4(sample, 0.0); // vec4 because need padding
		}
		const VkDeviceSize kernelBufferSize = sizeof(glm::vec4) * ssaoKernel_.size();
		kernel_.CreateGPUOnlyBuffer(
			ctx,
			kernelBufferSize,
			ssaoKernel_.data(),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	}
}

void ResourcesGBuffer::CreateSampler(VulkanContext& ctx, VkSampler* sampler)
{
	VkSamplerCreateInfo samplerInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.magFilter = VK_FILTER_NEAREST,
		.minFilter = VK_FILTER_NEAREST,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		.mipLodBias = 0.0f,
		.maxAnisotropy = 1.0f,
		.minLod = 0.0f,
		.maxLod = 1.0f,
		.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
	};
	VK_CHECK(vkCreateSampler(ctx.GetDevice(), &samplerInfo, nullptr, sampler));
}