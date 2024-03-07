#include "ResourcesShadow.h"
#include "Configs.h"

ResourcesShadow::ResourcesShadow() : device_(nullptr)
{
	for (size_t i = 0; i < views_.size(); ++i)
	{
		views_[i] = nullptr;
	}
}

ResourcesShadow::~ResourcesShadow()
{
	Destroy();
}

void ResourcesShadow::Destroy()
{
	shadowMap_.Destroy();

	for (size_t i = 0; i < views_.size(); ++i)
	{
		if (views_[i])
		{
			vkDestroyImageView(device_, views_[i], nullptr);
			views_[i] = nullptr;
		}
	}
}

void ResourcesShadow::CreateSingleShadowMap(VulkanContext& ctx)
{
	// Init shadow map
	shadowMap_.CreateDepthResources(
		ctx,
		ShadowConfig::DepthSize,
		ShadowConfig::DepthSize,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_USAGE_SAMPLED_BIT);
	shadowMap_.CreateDefaultSampler(
		ctx,
		0.f,
		1.f,
		VK_FILTER_LINEAR,
		VK_FILTER_LINEAR,
		VK_TRUE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	shadowMap_.SetDebugName(ctx, "Single_Layer_Shadow_Map");

	device_ = ctx.GetDevice();
}

void ResourcesShadow::CreateCascadeShadowMap(VulkanContext& ctx)
{
	// Init shadow map
	const VkFormat depthFormat = ctx.GetDepthFormat();
	shadowMap_.CreateImage(
		ctx,
		ShadowConfig::DepthSize,
		ShadowConfig::DepthSize,
		1u, // mip
		ShadowConfig::CascadeCount, // layer
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		0u, // VkImageCreateFlags
		VK_SAMPLE_COUNT_1_BIT); // always single sampled
	shadowMap_.CreateImageView(
		ctx,
		depthFormat,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		VK_IMAGE_VIEW_TYPE_2D_ARRAY,
		0u,
		1u, // mipCount
		0u,
		ShadowConfig::CascadeCount); // layerCount
	shadowMap_.CreateDefaultSampler(
		ctx,
		0.f,
		1.f,
		VK_FILTER_LINEAR,
		VK_FILTER_LINEAR,
		VK_TRUE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
	shadowMap_.SetDebugName(ctx, "Cascade_Shadow_Map");

	for (uint32_t i = 0; i < ShadowConfig::CascadeCount; ++i)
	{
		VulkanImage::CreateImageView(
			ctx,
			shadowMap_.image_,
			views_[i],
			depthFormat,
			VK_IMAGE_ASPECT_DEPTH_BIT,
			VK_IMAGE_VIEW_TYPE_2D_ARRAY,
			0u,
			1u, // mipCount
			i,
			1u);
	}

	shadowMap_.TransitionLayout(
		ctx,
		depthFormat,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	device_ = ctx.GetDevice();
}