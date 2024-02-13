#include "VulkanImage.h"
#include "VulkanBuffer.h"
#include "VulkanUtility.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int NumMipMap(int w, int h)
{
	int levels = 1;
	while ((w | h) >> levels)
	{
		levels += 1;
	}
	return levels;
}

void VulkanImage::Destroy(VkDevice device)
{
	if (vmaAllocator_ == nullptr)
	{
		return;
	}

	vkDestroySampler(device, defaultImageSampler_, nullptr);
	vkDestroyImageView(device, imageView_, nullptr);
	vmaDestroyImage(vmaAllocator_, image_, vmaAllocation_);
}

void VulkanImage::CreateImageResources(
	VulkanDevice& vkDev,
	const char* filename)
{
	CreateFromFile(vkDev, filename);
	GenerateMipmap(vkDev,
		mipCount_,
		width_,
		height_,
		VK_IMAGE_LAYOUT_UNDEFINED);
	CreateImageView(
		vkDev,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_2D,
		layerCount_,
		mipCount_);
	CreateDefaultSampler(vkDev,
		0.f, // minLod
		static_cast<float>(mipCount_)); // maxLod
}

void VulkanImage::CreateFromFile(
	VulkanDevice& vkDev,
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
		vkDev,
		pixels,
		texWidth,
		texHeight,
		NumMipMap(texWidth, texHeight),
		1, // layerCount
		VK_FORMAT_R8G8B8A8_UNORM);

	stbi_image_free(pixels);
}

void VulkanImage::CreateFromHDR(
	VulkanDevice& vkDev,
	const char* filename)
{
	stbi_set_flip_vertically_on_load(true);

	int texWidth, texHeight, texChannels;
	float* pixels = stbi_loadf(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	CreateImageFromData(
		vkDev,
		pixels,
		texWidth,
		texHeight,
		NumMipMap(texWidth, texHeight),
		1,
		VK_FORMAT_R32G32B32A32_SFLOAT);

	stbi_image_free(pixels);
}

void VulkanImage::CreateColorResources(
	VulkanDevice& vkDev, 
	uint32_t width, 
	uint32_t height,
	VkSampleCountFlagBits outputDiffuseSampleCount)
{
	VkFormat format = vkDev.GetSwapchainImageFormat();

	CreateImage(
		vkDev,
		width,
		height,
		1, // mip
		1, // layer
		format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		0u,
		outputDiffuseSampleCount);

	CreateImageView(
		vkDev,
		format,
		VK_IMAGE_ASPECT_COLOR_BIT);
	CreateDefaultSampler(vkDev);
}

void VulkanImage::CreateDepthResources(
	VulkanDevice& vkDev, 
	uint32_t width, 
	uint32_t height,
	VkSampleCountFlagBits outputDiffuseSampleCount)
{
	VkFormat depthFormat = vkDev.GetDepthFormat();

	CreateImage(
		vkDev,
		width,
		height,
		1, // mip
		1, // layer
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		0u,
		outputDiffuseSampleCount);

	CreateImageView(
		vkDev,
		depthFormat,
		VK_IMAGE_ASPECT_DEPTH_BIT);

	TransitionImageLayout(
		vkDev, 
		depthFormat, 
		VK_IMAGE_LAYOUT_UNDEFINED, 
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void VulkanImage::CreateImageFromData(
	VulkanDevice& vkDev,
	void* imageData,
	uint32_t texWidth,
	uint32_t texHeight,
	uint32_t mipmapCount,
	uint32_t layerCount,
	VkFormat texFormat,
	VkImageCreateFlags flags)
{
	CreateImage(
		vkDev, 
		texWidth, 
		texHeight, 
		mipmapCount,
		layerCount,
		texFormat, 
		VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VMA_MEMORY_USAGE_GPU_ONLY,
		flags);

	UpdateImage(vkDev, texWidth, texHeight, texFormat, layerCount, imageData);
}

void VulkanImage::CopyBufferToImage(
	VulkanDevice& vkDev,
	VkBuffer buffer,
	uint32_t width,
	uint32_t height,
	uint32_t layerCount)
{
	VkCommandBuffer commandBuffer = vkDev.BeginOneTimeGraphicsCommand();

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

	vkDev.EndOneTimeGraphicsCommand(commandBuffer);
}

void VulkanImage::CreateImage(
	VulkanDevice& vkDev,
	uint32_t width,
	uint32_t height,
	uint32_t mipCount,
	uint32_t layerCount,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags imageUsage,
	VmaMemoryUsage memoryUsage,
	VkImageCreateFlags flags,
	VkSampleCountFlagBits outputDiffuseSampleCount)
{
	width_ = width;
	height_ = height;
	mipCount_ = mipCount;
	layerCount_ = layerCount;
	imageFormat_ = format;
	multisampleCount_ = outputDiffuseSampleCount; // MSAA
	
	const VkImageCreateInfo imageInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = flags,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = VkExtent3D {.width = width_, .height = height_, .depth = 1 },
		.mipLevels = mipCount_,
		.arrayLayers = layerCount_,
		.samples = outputDiffuseSampleCount,
		.tiling = tiling,
		.usage = imageUsage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VmaAllocationCreateInfo allocinfo = {
		.usage = VMA_MEMORY_USAGE_GPU_ONLY,
		.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
	};

	vmaAllocator_ = vkDev.GetVMAAllocator();
	VK_CHECK(vmaCreateImage(
		vmaAllocator_,
		&imageInfo, 
		&allocinfo, 
		&image_, 
		&vmaAllocation_, 
		nullptr));
}

void VulkanImage::CreateImageView(
	VulkanDevice& vkDev, 
	VkFormat format, 
	VkImageAspectFlags aspectFlags, 
	VkImageViewType viewType, 
	uint32_t layerCount, 
	uint32_t mipCount)
{
	const VkImageViewCreateInfo viewInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = image_,
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
			.baseMipLevel = 0,
			.levelCount = mipCount,
			.baseArrayLayer = 0,
			.layerCount = layerCount
		}
	};

	VK_CHECK(vkCreateImageView(vkDev.GetDevice(), &viewInfo, nullptr, &imageView_));
}

void VulkanImage::CreateDefaultSampler(
	VulkanDevice& vkDev,
	float minLod,
	float maxLod,
	VkFilter minFilter,
	VkFilter maxFilter,
	VkSamplerAddressMode addressMode)
{
	CreateSampler(
		vkDev,
		defaultImageSampler_,
		minLod,
		maxLod,
		minFilter,
		maxFilter,
		addressMode
	);
}

void VulkanImage::CreateSampler(
	VulkanDevice& vkDev,
	VkSampler& sampler,
	float minLod,
	float maxLod,
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
		.minLod = minLod,
		.maxLod = maxLod,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE
	};

	VK_CHECK(vkCreateSampler(vkDev.GetDevice(), &samplerInfo, nullptr, &sampler));
}

void VulkanImage::UpdateImage(
	VulkanDevice& vkDev,
	uint32_t texWidth,
	uint32_t texHeight,
	VkFormat texFormat,
	uint32_t layerCount,
	const void* imageData,
	VkImageLayout sourceImageLayout)
{
	uint32_t bytesPerPixel = BytesPerTexFormat(texFormat);

	const VkDeviceSize layerSize = texWidth * texHeight * bytesPerPixel;
	const VkDeviceSize imageSize = layerSize * layerCount;

	VulkanBuffer stagingBuffer{};

	stagingBuffer.CreateBuffer(
		vkDev,
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_ONLY);

	stagingBuffer.UploadBufferData(vkDev, 0, imageData, imageSize);
	TransitionImageLayout(vkDev, texFormat, sourceImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layerCount);
	CopyBufferToImage(vkDev, stagingBuffer.buffer_, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), layerCount);
	TransitionImageLayout(vkDev, texFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, layerCount);

	stagingBuffer.Destroy();
}

void VulkanImage::TransitionImageLayout(VulkanDevice& vkDev,
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	uint32_t layerCount,
	uint32_t mipLevels)
{
	VkCommandBuffer commandBuffer = vkDev.BeginOneTimeGraphicsCommand();
	TransitionImageLayoutCommand(commandBuffer, format, oldLayout, newLayout, layerCount, mipLevels);
	vkDev.EndOneTimeGraphicsCommand(commandBuffer);
}

void VulkanImage::TransitionImageLayoutCommand(
	VkCommandBuffer commandBuffer,
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	uint32_t layerCount,
	uint32_t mipLevels)
{
	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = 0,
		.dstAccessMask = 0,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image_,
		.subresourceRange = VkImageSubresourceRange {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = mipLevels,
			.baseArrayLayer = 0,
			.layerCount = layerCount
		}
	};

	VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
		(format == VK_FORMAT_D16_UNORM) ||
		(format == VK_FORMAT_X8_D24_UNORM_PACK32) ||
		(format == VK_FORMAT_D32_SFLOAT) ||
		(format == VK_FORMAT_S8_UINT) ||
		(format == VK_FORMAT_D16_UNORM_S8_UINT) ||
		(format == VK_FORMAT_D24_UNORM_S8_UINT))
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (HasStencilComponent(format))
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && 
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && 
		newLayout == VK_IMAGE_LAYOUT_GENERAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && 
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	// Convert back from read-only to updateable
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && 
		newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	// Convert from updateable texture to shader read-only
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && 
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	// Convert depth texture from undefined state to depth-stencil buffer
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && 
		newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | 
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}

	// Wait for render pass to complete
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && 
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = 0; // VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = 0;
		/*
				sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		///		destinationStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
				destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		*/
		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	// Convert back from read-only to color attachment
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && 
		newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	// Convert from updateable texture to shader read-only
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && 
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	// Convert back from read-only to depth attachment
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && 
		newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	}
	// Convert from updateable depth texture to shader read-only
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && 
		newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	vkCmdPipelineBarrier(
		commandBuffer, // commandBuffer
		sourceStage, // srcStageMask
		destinationStage, // dstStageMask
		0, // dependencyFlags
		0, // memoryBarrierCount
		nullptr, // pMemoryBarriers
		0, // bufferMemoryBarrierCount
		nullptr, // pBufferMemoryBarriers
		1, // imageMemoryBarrierCount
		&barrier // pImageMemoryBarriers
	);
}

void VulkanImage::GenerateMipmap(
	VulkanDevice& vkDev,
	uint32_t maxMipLevels,
	uint32_t width,
	uint32_t height,
	VkImageLayout currentImageLayout
)
{
	VkCommandBuffer commandBuffer = vkDev.BeginOneTimeGraphicsCommand();

	VkImageSubresourceRange mipbaseRange{};
	mipbaseRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	mipbaseRange.baseMipLevel = 0u;
	mipbaseRange.levelCount = 1u; // The number of mipmap levels (starting from baseMipLevel) accessible to the view
	mipbaseRange.layerCount = layerCount_;

	CreateBarrier({
		commandBuffer, // cmdBuffer
		currentImageLayout, // oldLayout
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // newLayout
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStage
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // srcAccess
		VK_PIPELINE_STAGE_TRANSFER_BIT, // dstStage
		VK_ACCESS_TRANSFER_READ_BIT }, // dstAccess
		mipbaseRange);

	for (uint32_t i = 1; i < maxMipLevels; ++i)
	{
		VkImageBlit imageBlit{};

		// Source
		imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.srcSubresource.layerCount = layerCount_;
		imageBlit.srcSubresource.mipLevel = i - 1;
		imageBlit.srcOffsets[1].x = int32_t(width >> (i - 1));
		imageBlit.srcOffsets[1].y = int32_t(height >> (i - 1));
		imageBlit.srcOffsets[1].z = 1;

		// Destination
		imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.dstSubresource.layerCount = layerCount_;
		imageBlit.dstSubresource.mipLevel = i;
		imageBlit.dstOffsets[1].x = int32_t(width >> i);
		imageBlit.dstOffsets[1].y = int32_t(height >> i);
		imageBlit.dstOffsets[1].z = 1;

		VkImageSubresourceRange mipSubRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = i,
			.levelCount = 1,
			.layerCount = layerCount_
		};

		//  Transiton current mip level to transfer dest
		CreateBarrier({
			.commandBuffer = commandBuffer,
			.oldLayout = currentImageLayout,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.sourceAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
			.destinationAccess = VK_ACCESS_TRANSFER_WRITE_BIT },
			mipSubRange);

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
		CreateBarrier({
			.commandBuffer = commandBuffer,
			.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			.sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
			.sourceAccess = VK_ACCESS_TRANSFER_WRITE_BIT,
			.destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT,
			.destinationAccess = VK_ACCESS_TRANSFER_READ_BIT },
			mipSubRange);
	}

	// Convention is to change the layout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	CreateBarrier({ 
		.commandBuffer = commandBuffer,
		.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		.sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.sourceAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		.destinationAccess = VK_ACCESS_SHADER_READ_BIT 
	});

	vkDev.EndOneTimeGraphicsCommand(commandBuffer);
}

void VulkanImage::CreateBarrier(ImageBarrierCreateInfo info)
{
	VkImageSubresourceRange subresourceRange =
	{
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0u,
		.levelCount = mipCount_,
		.baseArrayLayer = 0u,
		.layerCount = layerCount_
	};
	CreateBarrier(info, subresourceRange);
}

void VulkanImage::CreateBarrier(ImageBarrierCreateInfo info, VkImageSubresourceRange subresourceRange)
{
	VkImageMemoryBarrier barrier =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = info.sourceAccess,
		.dstAccessMask = info.destinationAccess,
		.oldLayout = info.oldLayout,
		.newLayout = info.newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image_,
		.subresourceRange = subresourceRange
	};

	vkCmdPipelineBarrier(
		info.commandBuffer, // commandBuffer
		info.sourceStage, // srcStageMask
		info.destinationStage, // dstStageMask
		0u, // dependencyFlags
		0u, // memoryBarrierCount
		nullptr, // pMemoryBarriers
		0u, // bufferMemoryBarrierCount
		nullptr, // pBufferMemoryBarriers
		1u, // imageMemoryBarrierCount
		&barrier // pImageMemoryBarriers
	);
}

bool VulkanImage::HasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
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
		return 4;
	case VK_FORMAT_R32G32_SFLOAT:
		return 2 * sizeof(float);
	case VK_FORMAT_R16G16_SNORM:
		return 4;
	case VK_FORMAT_B8G8R8A8_UNORM:
		return 4;
	case VK_FORMAT_R8G8B8A8_UNORM:
		return 4;
	case VK_FORMAT_R16G16B16A16_SFLOAT:
		return 4 * sizeof(uint16_t);
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		return 4 * sizeof(float);
	default:
		break;
	}
	return 0;
}

VkDescriptorImageInfo VulkanImage::GetDescriptorImageInfo()
{
	return
	{
		defaultImageSampler_,
		imageView_,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};
}

void VulkanImage::SetDebugName(VulkanDevice& vkDev, const std::string& debugName)
{
	vkDev.SetVkObjectName(image_, VK_OBJECT_TYPE_IMAGE, debugName.c_str());
}