#include "VulkanImage.h"
#include "VulkanBuffer.h"
#include "VulkanUtility.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "VulkanBarrier.h"

void VulkanImage::Destroy()
{
	if (defaultImageSampler_)
	{
		vkDestroySampler(device_, defaultImageSampler_, nullptr);
		defaultImageSampler_ = nullptr;
	}

	if (imageView_)
	{
		vkDestroyImageView(device_, imageView_, nullptr);
		imageView_ = nullptr;
	}

	if (vmaAllocation_)
	{
		vmaDestroyImage(vmaAllocator_, image_, vmaAllocation_);
		image_ = nullptr;
		vmaAllocation_ = nullptr;
	}
}

void VulkanImage::CreateImageResources(
	VulkanContext& ctx,
	const char* filename)
{
	CreateFromFile(ctx, filename);
	GenerateMipmap(ctx,
		mipCount_,
		width_,
		height_,
		VK_IMAGE_LAYOUT_UNDEFINED);
	CreateImageView(
		ctx,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_2D,
		0u,
		mipCount_,
		0u,
		layerCount_);
	CreateDefaultSampler(ctx,
		0.f, // minLod
		static_cast<float>(mipCount_)); // maxLod
}

void VulkanImage::CreateImageResources(
	VulkanContext& ctx,
	void* data,
	int width,
	int height)
{
	CreateImageFromData(
		ctx,
		data,
		width,
		height,
		Utility::MipMapCount(width, height),
		1u, // layerCount
		VK_FORMAT_R8G8B8A8_UNORM);
	GenerateMipmap(ctx,
		mipCount_,
		width_,
		height_,
		VK_IMAGE_LAYOUT_UNDEFINED);
	CreateImageView(
		ctx,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_2D,
		0u,
		mipCount_,
		0u,
		layerCount_);
	CreateDefaultSampler(ctx,
		0.f, // minLod
		static_cast<float>(mipCount_)); // maxLod
}

void VulkanImage::CreateFromFile(
	VulkanContext& ctx,
	const char* filename)
{
	stbi_set_flip_vertically_on_load(false);

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels)
	{
		std::cerr << "Failed to load image " << filename << '\n';
	}

	CreateImageFromData(
		ctx,
		pixels,
		texWidth,
		texHeight,
		Utility::MipMapCount(texWidth, texHeight),
		1, // layerCount
		VK_FORMAT_R8G8B8A8_UNORM);

	stbi_image_free(pixels);
}

void VulkanImage::CreateFromHDR(
	VulkanContext& ctx,
	const char* filename)
{
	stbi_set_flip_vertically_on_load(true);
	int texWidth, texHeight, texChannels;
	float* pixels = stbi_loadf(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	CreateImageFromData(
		ctx,
		pixels,
		texWidth,
		texHeight,
		Utility::MipMapCount(texWidth, texHeight),
		1,
		VK_FORMAT_R32G32B32A32_SFLOAT);
	stbi_image_free(pixels);
}

void VulkanImage::CreateColorResources(
	VulkanContext& ctx, 
	uint32_t width, 
	uint32_t height,
	VkSampleCountFlagBits sampleCount)
{
	const VkFormat format = ctx.GetSwapchainImageFormat();
	CreateImage(
		ctx,
		width,
		height,
		1u, // mip
		1u, // layer
		format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		0u,
		sampleCount);
	CreateImageView(
		ctx,
		format,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_2D,
		0u,
		1u, // layerCount
		0u,
		1u);
	CreateDefaultSampler(ctx);
}

void VulkanImage::CreateDepthResources(
	VulkanContext& ctx, 
	uint32_t width, 
	uint32_t height,
	VkSampleCountFlagBits sampleCount,
	VkImageUsageFlags additionalUsage)
{
	const VkFormat depthFormat = ctx.GetDepthFormat();
	CreateImage(
		ctx,
		width,
		height,
		1u, // mip
		1u, // layer
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | additionalUsage,
		VMA_MEMORY_USAGE_GPU_ONLY,
		0u,
		sampleCount);
	CreateImageView(
		ctx,
		depthFormat,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		VK_IMAGE_VIEW_TYPE_2D,
		0u,
		1u, // layerCount
		0u,
		1u); // mipCount
	TransitionLayout(
		ctx, 
		depthFormat, 
		VK_IMAGE_LAYOUT_UNDEFINED, 
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void VulkanImage::CreateImageFromData(
	VulkanContext& ctx,
	void* imageData,
	uint32_t texWidth,
	uint32_t texHeight,
	uint32_t mipmapCount,
	uint32_t layerCount,
	VkFormat texFormat,
	VkImageCreateFlags flags)
{
	CreateImage(
		ctx, 
		texWidth, 
		texHeight, 
		mipmapCount,
		layerCount,
		texFormat, 
		VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		flags);
	UpdateImage(ctx, texWidth, texHeight, texFormat, layerCount, imageData);
}

void VulkanImage::CopyBufferToImage(
	VulkanContext& ctx,
	VkBuffer buffer,
	uint32_t width,
	uint32_t height,
	uint32_t layerCount)
{
	VkCommandBuffer commandBuffer = ctx.BeginOneTimeGraphicsCommand();
	const VkBufferImageCopy region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = VkImageSubresourceLayers {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = layerCount
		},
		.imageOffset = VkOffset3D {.x = 0, .y = 0, .z = 0 },
		.imageExtent = VkExtent3D {.width = width, .height = height, .depth = 1 }
	};
	vkCmdCopyBufferToImage(commandBuffer, buffer, image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	ctx.EndOneTimeGraphicsCommand(commandBuffer);
}

void VulkanImage::CreateImage(
	VulkanContext& ctx,
	uint32_t width,
	uint32_t height,
	uint32_t mipCount,
	uint32_t layerCount,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags imageUsage,
	VmaMemoryUsage memoryUsage,
	VkImageCreateFlags flags,
	VkSampleCountFlagBits sampleCount)
{
	width_ = width;
	height_ = height;
	mipCount_ = mipCount;
	layerCount_ = layerCount;
	imageFormat_ = format;
	multisampleCount_ = sampleCount; // MSAA
	
	const VkImageCreateInfo imageInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = flags,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = VkExtent3D {.width = width_, .height = height_, .depth = 1 },
		.mipLevels = mipCount_,
		.arrayLayers = layerCount_,
		.samples = sampleCount,
		.tiling = tiling,
		.usage = imageUsage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VmaAllocationCreateInfo allocinfo = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
		.requiredFlags = static_cast<VkMemoryPropertyFlags>(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
	};

	device_ = ctx.GetDevice();
	vmaAllocator_ = ctx.GetVMAAllocator();

	VK_CHECK(vmaCreateImage(
		vmaAllocator_,
		&imageInfo, 
		&allocinfo, 
		&image_, 
		&vmaAllocation_, 
		nullptr));
}

void VulkanImage::CreateImageView(
	VulkanContext& ctx, 
	VkFormat format, 
	VkImageAspectFlags aspectFlags, 
	VkImageViewType viewType, 
	uint32_t mipLevel,
	uint32_t mipCount,
	uint32_t layerLevel,
	uint32_t layerCount)
{
	CreateImageView(ctx, image_, imageView_, format, aspectFlags, viewType, mipLevel, mipCount, layerLevel, layerCount);
}

void VulkanImage::CreateImageView(
	VulkanContext& ctx,
	VkImage image,
	VkImageView& view,
	VkFormat format,
	VkImageAspectFlags aspectFlags,
	VkImageViewType viewType,
	uint32_t mipLevel,
	uint32_t mipCount,
	uint32_t layerLevel,
	uint32_t layerCount)
{
	const VkImageViewCreateInfo viewInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = image,
		.viewType = viewType,
		.format = format,
		.components =
		{
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
		},
		.subresourceRange =
		{
			.aspectMask = aspectFlags,
			.baseMipLevel = mipLevel,
			.levelCount = mipCount,
			.baseArrayLayer = layerLevel,
			.layerCount = layerCount
		}
	};
	VK_CHECK(vkCreateImageView(ctx.GetDevice(), &viewInfo, nullptr, &view));
}

void VulkanImage::CreateDefaultSampler(
	VulkanContext& ctx,
	float minLod,
	float maxLod,
	VkFilter minFilter,
	VkFilter maxFilter,
	VkBool32 anisoptropy,
	VkSamplerAddressMode addressMode)
{
	CreateSampler(
		ctx,
		defaultImageSampler_,
		minLod,
		maxLod,
		minFilter,
		maxFilter,
		anisoptropy,
		addressMode
	);
}

void VulkanImage::CreateSampler(
	VulkanContext& ctx,
	VkSampler& sampler,
	float minLod,
	float maxLod,
	VkFilter minFilter,
	VkFilter maxFilter,
	VkBool32 anisoptropy,
	VkSamplerAddressMode addressMode)
{
	const VkSamplerCreateInfo samplerInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = addressMode,
		.addressModeV = addressMode,
		.addressModeW = addressMode,
		.mipLodBias = 0.0f,
		.anisotropyEnable = anisoptropy,
		.maxAnisotropy = 1.0f,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = minLod,
		.maxLod = maxLod,
		.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
		.unnormalizedCoordinates = VK_FALSE
	};
	VK_CHECK(vkCreateSampler(ctx.GetDevice(), &samplerInfo, nullptr, &sampler));
}

void VulkanImage::UpdateImage(
	VulkanContext& ctx,
	uint32_t texWidth,
	uint32_t texHeight,
	VkFormat texFormat,
	uint32_t layerCount,
	const void* imageData,
	VkImageLayout sourceImageLayout)
{
	const uint32_t bytesPerPixel = BytesPerTexFormat(texFormat);

	const VkDeviceSize layerSize = texWidth * texHeight * bytesPerPixel;
	const VkDeviceSize imageSize = layerSize * layerCount;

	VulkanBuffer stagingBuffer{};

	stagingBuffer.CreateBuffer(
		ctx,
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_ONLY);

	stagingBuffer.UploadBufferData(ctx, imageData, imageSize);
	TransitionLayout(ctx, texFormat, sourceImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0u, 1u, 0u, layerCount);
	CopyBufferToImage(ctx, stagingBuffer.buffer_, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), layerCount);
	TransitionLayout(ctx, texFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0u, 1u, 0u, layerCount);

	stagingBuffer.Destroy();
}

void VulkanImage::TransitionLayout(
	VulkanContext& ctx,
	VkImageLayout oldLayout,
	VkImageLayout newLayout)
{
	TransitionLayout(ctx, imageFormat_, oldLayout, newLayout, 0u, mipCount_, 0u, layerCount_);
}

void VulkanImage::TransitionLayout(
	VulkanContext& ctx,
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	uint32_t mipLevel,
	uint32_t mipCount,
	uint32_t layerLevel,
	uint32_t layerCount)
{
	VkCommandBuffer commandBuffer = ctx.BeginOneTimeGraphicsCommand();
	TransitionLayoutCommand(commandBuffer, 
		image_, 
		format, 
		oldLayout, 
		newLayout, 
		mipLevel, 
		mipCount,
		layerLevel,
		layerCount);
	ctx.EndOneTimeGraphicsCommand(commandBuffer);
}

void VulkanImage::TransitionLayoutCommand(
	VkCommandBuffer commandBuffer,
	VkImage image,
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	uint32_t mipLevel,
	uint32_t mipCount,
	uint32_t layerLevel,
	uint32_t layerCount)
{
	VkImageMemoryBarrier2 barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.pNext = nullptr,
		.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, // Default
		.srcAccessMask = 0, // Set to the correct value
		.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, // Default
		.dstAccessMask = 0, // Set to the correct value
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = VkImageSubresourceRange {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = mipLevel,
			.levelCount = mipCount,
			.baseArrayLayer = layerLevel,
			.layerCount = layerCount
		}
	};

	// If depth
	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
		(format == VK_FORMAT_D16_UNORM) ||
		(format == VK_FORMAT_X8_D24_UNORM_PACK32) ||
		(format == VK_FORMAT_D32_SFLOAT) ||
		(format == VK_FORMAT_S8_UINT) ||
		(format == VK_FORMAT_D16_UNORM_S8_UINT) ||
		(format == VK_FORMAT_D24_UNORM_S8_UINT))
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		// If stencil
		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	// We have to set the access masks and stages
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT; // This is not VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
	}
	// Convert depth texture from undefined state to depth-stencil buffer
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
			VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT;
	}
	// Swapchain
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = 0;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	// Convert back from read-only to updateable
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
	}
	// Wait for render pass to complete
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = 0; 
		barrier.dstAccessMask = 0;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	}
	// Convert back from read-only to color attachment
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	// Convert back from read-only to depth attachment
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
	}
	// Convert from updateable texture to shader read-only
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	}
	// Swapchain
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = 0;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
	}
	// Convert from updateable texture to shader read-only
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	}
	// Convert from updateable depth texture to shader read-only
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

		barrier.srcStageMask = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	}

	VulkanBarrier::CreateImageBarrier(commandBuffer, &barrier, 1u);
}

// TODO This function uses CreateBarrier() instead on TransitionLayout()
void VulkanImage::GenerateMipmap(
	VulkanContext& ctx,
	uint32_t maxMipLevels,
	uint32_t width,
	uint32_t height,
	VkImageLayout currentImageLayout
)
{
	VkCommandBuffer commandBuffer = ctx.BeginOneTimeGraphicsCommand();

	VulkanBarrier::CreateImageBarrier(
		{
			.commandBuffer = commandBuffer,
			.oldLayout = currentImageLayout,
			.sourceStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			.sourceAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			.destinationStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			.destinationAccess = VK_ACCESS_2_TRANSFER_READ_BIT
		},
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0u,
			.levelCount = 1u, // The number of mipmap levels (starting from baseMipLevel) accessible to the view
			.baseArrayLayer = 0,
			.layerCount = layerCount_
		},
		image_
		);

	for (uint32_t i = 1; i < maxMipLevels; ++i)
	{
		// TODO use VkImageBlit2
		VkImageBlit imageBlit{};

		// Source
		imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.srcSubresource.layerCount = layerCount_;
		imageBlit.srcSubresource.mipLevel = i - 1;
		imageBlit.srcOffsets[1].x = static_cast<int32_t>(width >> (i - 1));
		imageBlit.srcOffsets[1].y = static_cast<int32_t>(height >> (i - 1));
		imageBlit.srcOffsets[1].z = 1;

		// Destination
		imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.dstSubresource.layerCount = layerCount_;
		imageBlit.dstSubresource.mipLevel = i;
		imageBlit.dstOffsets[1].x = static_cast<int32_t>(width >> i);
		imageBlit.dstOffsets[1].y = static_cast<int32_t>(height >> i);
		imageBlit.dstOffsets[1].z = 1;

		VkImageSubresourceRange subRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = i,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = layerCount_
		};

		// Transition current mip level to transfer dest
		VulkanBarrier::CreateImageBarrier(
			{
				.commandBuffer = commandBuffer,
				.oldLayout = currentImageLayout,
				.sourceStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
				.sourceAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.destinationStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				.destinationAccess = VK_ACCESS_2_TRANSFER_WRITE_BIT
			},
			subRange,
			image_);

		vkCmdBlitImage(
			commandBuffer,
			image_,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image_,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageBlit,
			VK_FILTER_LINEAR);

		// Transition back
		VulkanBarrier::CreateImageBarrier(
			{
				.commandBuffer = commandBuffer,
				.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.sourceStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				.sourceAccess = VK_ACCESS_2_TRANSFER_WRITE_BIT,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				.destinationStage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
				.destinationAccess = VK_ACCESS_2_TRANSFER_READ_BIT
			},
			subRange,
			image_);
	}

	// Convention is to change the layout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	VulkanBarrier::CreateImageBarrier({ 
		.commandBuffer = commandBuffer,
		.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		.sourceStage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
		.sourceAccess = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
		.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.destinationStage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
		.destinationAccess = VK_ACCESS_2_SHADER_READ_BIT
	},
	{
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = mipCount_,
		.baseArrayLayer = 0,
		.layerCount = layerCount_
	},
	image_
	);

	ctx.EndOneTimeGraphicsCommand(commandBuffer);
}

uint32_t VulkanImage::BytesPerTexFormat(VkFormat fmt)
{
	switch (fmt)
	{
	case VK_FORMAT_R8_SINT:
	case VK_FORMAT_R8_UNORM:
		return 1;
	case VK_FORMAT_R16_SFLOAT:
		return 2;
	case VK_FORMAT_R16G16_SFLOAT:
	case VK_FORMAT_R16G16_SNORM:
	case VK_FORMAT_B8G8R8A8_UNORM:
	case VK_FORMAT_R8G8B8A8_UNORM:
		return 4;
	case VK_FORMAT_R32G32_SFLOAT:
		return 2 * sizeof(float);
	case VK_FORMAT_R16G16B16A16_SFLOAT:
		return 4 * sizeof(uint16_t);
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		return 4 * sizeof(float);
	default:
		break;
	}
	return 0;
}

VkDescriptorImageInfo VulkanImage::GetDescriptorImageInfo() const
{
	return
	{
		defaultImageSampler_,
		imageView_,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};
}

void VulkanImage::SetDebugName(VulkanContext& ctx, const std::string& debugName)
{
	ctx.SetVkObjectName(image_, VK_OBJECT_TYPE_IMAGE, debugName.c_str());
}