#include "VulkanImage.h"
#include "VulkanBuffer.h"
#include "VulkanUtility.h"

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
	vkDestroyImageView(device, imageView_, nullptr);
	vkDestroyImage(device, image_, nullptr);
	vkFreeMemory(device, imageMemory_, nullptr);
}

void VulkanImage::CreateDepthResources(VulkanDevice& vkDev, uint32_t width, uint32_t height)
{
	VkFormat depthFormat = FindDepthFormat(vkDev.GetPhysicalDevice());

	CreateImage(vkDev.GetDevice(),
		vkDev.GetPhysicalDevice(),
		width,
		height,
		1, // mip
		1, // layer
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	CreateImageView(vkDev.GetDevice(),
		depthFormat,
		VK_IMAGE_ASPECT_DEPTH_BIT);

	TransitionImageLayout(vkDev, 
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
		vkDev.GetDevice(), 
		vkDev.GetPhysicalDevice(), 
		texWidth, 
		texHeight, 
		mipmapCount,
		layerCount,
		texFormat, 
		VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
		flags);

	UpdateTextureImage(vkDev, texWidth, texHeight, texFormat, layerCount, imageData);
}

void VulkanImage::CopyBufferToImage(
	VulkanDevice& vkDev,
	VkBuffer buffer,
	uint32_t width,
	uint32_t height,
	uint32_t layerCount)
{
	VkCommandBuffer commandBuffer = vkDev.BeginSingleTimeCommands();

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

	vkDev.EndSingleTimeCommands(commandBuffer);
}

void VulkanImage::CreateImage(
	VkDevice device,
	VkPhysicalDevice physicalDevice,
	uint32_t width,
	uint32_t height,
	uint32_t mipCount,
	uint32_t layerCount,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties,
	VkImageCreateFlags flags)
{
	width_ = width;
	height_ = height;
	mipCount_ = mipCount;
	layerCount_ = layerCount;
	imageFormat_ = format;
	
	const VkImageCreateInfo imageInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = flags,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = VkExtent3D {.width = width_, .height = height_, .depth = 1 },
		.mipLevels = mipCount_,
		.arrayLayers = layerCount_,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = tiling,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VK_CHECK(vkCreateImage(device, &imageInfo, nullptr, &image_));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image_, &memRequirements);

	const VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties)
	};

	VK_CHECK(vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory_));

	vkBindImageMemory(device, image_, imageMemory_, 0);
}

void VulkanImage::CreateImageView(VkDevice device, 
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

	VK_CHECK(vkCreateImageView(device, &viewInfo, nullptr, &imageView_));
}

uint32_t VulkanImage::FindMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(device, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	return 0xFFFFFFFF;
}

VkFormat VulkanImage::FindDepthFormat(VkPhysicalDevice device)
{
	return FindSupportedFormat(
		device,
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

VkFormat VulkanImage::FindSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(device, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("Failed to find supported format!\n");
}

void VulkanImage::UpdateTextureImage(
	VulkanDevice& vkDev,
	uint32_t texWidth,
	uint32_t texHeight,
	VkFormat texFormat,
	uint32_t layerCount,
	const void* imageData,
	VkImageLayout sourceImageLayout)
{
	uint32_t bytesPerPixel = BytesPerTexFormat(texFormat);

	VkDeviceSize layerSize = texWidth * texHeight * bytesPerPixel;
	VkDeviceSize imageSize = layerSize * layerCount;

	VulkanBuffer stagingBuffer{};

	stagingBuffer.CreateBuffer(
		vkDev.GetDevice(),
		vkDev.GetPhysicalDevice(),
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	stagingBuffer.UploadBufferData(vkDev, 0, imageData, imageSize);
	TransitionImageLayout(vkDev, texFormat, sourceImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layerCount);
	CopyBufferToImage(vkDev, stagingBuffer.buffer_, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), layerCount);
	TransitionImageLayout(vkDev, texFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, layerCount);

	stagingBuffer.Destroy(vkDev.GetDevice());
}

void VulkanImage::TransitionImageLayout(VulkanDevice& vkDev,
	VkFormat format,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	uint32_t layerCount,
	uint32_t mipLevels)
{
	VkCommandBuffer commandBuffer = vkDev.BeginSingleTimeCommands();

	TransitionImageLayoutCmd(commandBuffer, format, oldLayout, newLayout, layerCount, mipLevels);

	vkDev.EndSingleTimeCommands(commandBuffer);
}

void VulkanImage::TransitionImageLayoutCmd(
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

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

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

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	/* Convert back from read-only to updateable */
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	/* Convert from updateable texture to shader read-only */
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	/* Convert depth texture from undefined state to depth-stencil buffer */
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}

	/* Wait for render pass to complete */
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
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

	/* Convert back from read-only to color attachment */
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	/* Convert from updateable texture to shader read-only */
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	/* Convert back from read-only to depth attachment */
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	}
	/* Convert from updateable depth texture to shader read-only */
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, 
		destinationStage,
		0,
		0, 
		nullptr,
		0, 
		nullptr,
		1, 
		&barrier
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
	VkCommandBuffer commandBuffer = vkDev.BeginSingleTimeCommands();

	VkImageSubresourceRange mipbaseRange{};
	mipbaseRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	mipbaseRange.baseMipLevel = 0u;
	mipbaseRange.levelCount = 1u; // The number of mipmap levels (starting from baseMipLevel) accessible to the view
	mipbaseRange.layerCount = layerCount_;

	{
		CreateBarrier(
			commandBuffer, // cmdBuffer
			currentImageLayout, // oldLayout
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // newLayout
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStage
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // srcAccess
			VK_PIPELINE_STAGE_TRANSFER_BIT, // dstStage
			VK_ACCESS_TRANSFER_READ_BIT, // dstAccess
			mipbaseRange);
	}

	for (uint32_t i = 1; i < maxMipLevels; ++i)
	{
		VkImageBlit imageBlit{};

		// Source
		imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.srcSubresource.layerCount = 6u;
		imageBlit.srcSubresource.mipLevel = i - 1;
		imageBlit.srcOffsets[1].x = int32_t(width >> (i - 1));
		imageBlit.srcOffsets[1].y = int32_t(height >> (i - 1));
		imageBlit.srcOffsets[1].z = 1;

		// Destination
		imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.dstSubresource.layerCount = 6u;
		imageBlit.dstSubresource.mipLevel = i;
		imageBlit.dstOffsets[1].x = int32_t(width >> i);
		imageBlit.dstOffsets[1].y = int32_t(height >> i);
		imageBlit.dstOffsets[1].z = 1;

		VkImageSubresourceRange mipSubRange = {};
		mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		mipSubRange.baseMipLevel = i;
		mipSubRange.levelCount = 1;
		mipSubRange.layerCount = 6u;

		//  Transiton current mip level to transfer dest
		CreateBarrier(
			commandBuffer,
			currentImageLayout, // oldLayout
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // newLayout
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStage
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // srcAccess
			VK_PIPELINE_STAGE_TRANSFER_BIT, // dstStage
			VK_ACCESS_TRANSFER_WRITE_BIT, // dstAccess
			mipSubRange); // subresourceRange

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
		CreateBarrier(
			commandBuffer,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // oldLayout
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // newLayout
			VK_PIPELINE_STAGE_TRANSFER_BIT, // srcStage
			VK_ACCESS_TRANSFER_WRITE_BIT, // srcAccess
			VK_PIPELINE_STAGE_TRANSFER_BIT, // dstStage
			VK_ACCESS_TRANSFER_READ_BIT, // dstAccess
			mipSubRange);
	}

	{
		// Convention is to change the layout to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		CreateBarrier(commandBuffer,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // oldLayout
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // newLayout
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStage
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, // srcAccess
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // dstStage
			VK_ACCESS_SHADER_READ_BIT // dstAccess
		);
	}

	vkDev.EndSingleTimeCommands(commandBuffer);
}

void VulkanImage::CreateBarrier(
	VkCommandBuffer cmdBuffer,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	VkPipelineStageFlags srcStage,
	VkAccessFlags srcAccess,
	VkPipelineStageFlags dstStage,
	VkAccessFlags dstAccess)
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image_;
	barrier.subresourceRange = 
	{
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0u,
		.levelCount = mipCount_,
		.baseArrayLayer = 0u,
		.layerCount = layerCount_
	};
	barrier.srcAccessMask = srcAccess;
	barrier.dstAccessMask = dstAccess;

	vkCmdPipelineBarrier(
		cmdBuffer,
		srcStage,
		dstStage,
		0u,
		0u,
		nullptr,
		0u,
		nullptr,
		1u,
		&barrier
	);
}

void VulkanImage::CreateBarrier(
	VkCommandBuffer cmdBuffer,
	VkImageLayout oldLayout,
	VkImageLayout newLayout,
	VkPipelineStageFlags srcStage,
	VkAccessFlags srcAccess,
	VkPipelineStageFlags dstStage,
	VkAccessFlags dstAccess,
	VkImageSubresourceRange subresourceRange)
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image_;
	barrier.subresourceRange = subresourceRange;
	barrier.srcAccessMask = srcAccess;
	barrier.dstAccessMask = dstAccess;

	vkCmdPipelineBarrier(
		cmdBuffer,
		srcStage, 
		dstStage,
		0u,
		0u, 
		nullptr,
		0u, 
		nullptr,
		1u, 
		&barrier
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