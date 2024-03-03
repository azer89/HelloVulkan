#ifndef RESOURCES_SHADOW
#define RESOURCES_SHADOW

#include "VulkanContext.h"
#include "VulkanImage.h"

struct ResourcesShadow
{
public:
	VulkanImage shadowMap_;

	void CreateSingleShadowMap(VulkanContext& ctx)
	{
		// Init shadow map
		shadowMap_.CreateDepthResources(
			ctx,
			ShadowConfig::DepthSize,
			ShadowConfig::DepthSize,
			1u,// layerCount
			VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_USAGE_SAMPLED_BIT);
		shadowMap_.CreateDefaultSampler(
			ctx,
			0.f,
			1.f,
			VK_FILTER_LINEAR,
			VK_FILTER_LINEAR,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
		shadowMap_.SetDebugName(ctx, "Single_Layer_Shadow_Map");

		device_ = ctx.GetDevice();
	}

	void Destroy()
	{
		shadowMap_.Destroy();
	}

private:
	VkDevice device_;
};

#endif