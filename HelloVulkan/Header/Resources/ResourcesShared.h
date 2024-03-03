#ifndef RESOURCES_SHARED
#define RESOURCES_SHARED

#include "VulkanContext.h"
#include "VulkanImage.h"

struct ResourcesShared
{
public:
	ResourcesShared() = default;
	~ResourcesShared()
	{
		Destroy();
	}

	void Create(VulkanContext& ctx)
	{
		depthImage_.Destroy();
		multiSampledColorImage_.Destroy();
		singleSampledColorImage_.Destroy();

		const VkSampleCountFlagBits msaaSamples = ctx.GetMSAASampleCount();
		const uint32_t width = ctx.GetSwapchainWidth();
		const uint32_t height = ctx.GetSwapchainHeight();

		// Depth attachment (OnScreen and offscreen)
		depthImage_.CreateDepthResources(
			ctx,
			width,
			height,
			1u, // layerCount
			msaaSamples);
		depthImage_.SetDebugName(ctx, "Depth_Image");

		// Color attachments
		// Multi-sampled (MSAA)
		multiSampledColorImage_.CreateColorResources(
			ctx,
			width,
			height,
			msaaSamples);
		multiSampledColorImage_.SetDebugName(ctx, "Multisampled_Color_Image");

		// Single-sampled
		singleSampledColorImage_.CreateColorResources(
			ctx,
			width,
			height);
		singleSampledColorImage_.SetDebugName(ctx, "Singlesampled_Color_Image");
	}

	void Destroy()
	{
		depthImage_.Destroy();
		multiSampledColorImage_.Destroy();
		singleSampledColorImage_.Destroy();
	}

public:
	VulkanImage multiSampledColorImage_;
	VulkanImage singleSampledColorImage_;
	VulkanImage depthImage_;
};

#endif