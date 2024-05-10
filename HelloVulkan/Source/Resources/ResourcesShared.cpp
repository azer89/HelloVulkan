#include "ResourcesShared.h"

ResourcesShared::ResourcesShared()
{
}

ResourcesShared::~ResourcesShared()
{
	Destroy();
}

void ResourcesShared::Create(VulkanContext& ctx)
{
	depthImage_.Destroy();
	multiSampledColorImage_.Destroy();
	singleSampledColorImage_.Destroy();

	const VkSampleCountFlagBits msaaSamples = ctx.GetMSAASampleCount();
	const uint32_t width = ctx.GetSwapchainWidth();
	const uint32_t height = ctx.GetSwapchainHeight();

	// Depth attachment (OnScreen and offscreen)
	depthImage_.CreateDepthAttachment(
		ctx,
		width,
		height,
		1u, // layerCount
		msaaSamples);
	depthImage_.SetDebugName(ctx, "Depth_Image");

	// Color attachments
	// Multi-sampled (MSAA)
	multiSampledColorImage_.CreateColorAttachment(
		ctx,
		width,
		height,
		msaaSamples);
	multiSampledColorImage_.SetDebugName(ctx, "Multisampled_Color_Image");

	// Single-sampled
	singleSampledColorImage_.CreateColorAttachment(
		ctx,
		width,
		height);
	singleSampledColorImage_.SetDebugName(ctx, "Singlesampled_Color_Image");
}

void ResourcesShared::Destroy()
{
	depthImage_.Destroy();
	multiSampledColorImage_.Destroy();
	singleSampledColorImage_.Destroy();
}

void ResourcesShared::OnWindowResized(VulkanContext& ctx)
{
	Create(ctx);
}