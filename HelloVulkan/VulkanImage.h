#ifndef VULKAN_IMAGE
#define VULKAN_IMAGE

#include "volk.h"

#include "VulkanDevice.h"

int NumMipMap(int w, int h);

class VulkanImage
{
public:
	VkImage image_;
	VkDeviceMemory imageMemory_;
	VkImageView imageView_;

	uint32_t width_;
	uint32_t height_;
	uint32_t mipCount_;
	uint32_t layerCount_;
	VkFormat imageFormat_;

public:

	VulkanImage() :
		image_(nullptr),
		imageMemory_(nullptr),
		imageView_(nullptr),
		width_(0),
		height_(0),
		mipCount_(0),
		layerCount_(0),
		imageFormat_(VK_FORMAT_UNDEFINED)
	{
	}

	bool CreateImage(
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
		VkImageCreateFlags flags = 0);

	bool CreateImageFromData(
		VulkanDevice& vkDev,
		void* imageData,
		uint32_t texWidth,
		uint32_t texHeight,
		uint32_t mipmapCount,
		uint32_t layerCount,
		VkFormat texFormat,
		VkImageCreateFlags flags = 0);

	bool CreateDepthResources(VulkanDevice& vkDev, uint32_t width, uint32_t height);

	void Destroy(VkDevice device);

	bool CreateImageView(VkDevice device,
		VkFormat format,
		VkImageAspectFlags aspectFlags,
		VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D,
		uint32_t layerCount = 1,
		uint32_t mipCount = 1);

	void CopyBufferToImage(
		VulkanDevice& vkDev,
		VkBuffer buffer,
		uint32_t width,
		uint32_t height,
		uint32_t layerCount = 1);

	void CreateBarrier(
		VkCommandBuffer _cmdBuffer, 
		VkImageLayout oldLayout, 
		VkImageLayout newLayout,
		VkPipelineStageFlags _srcStage, 
		VkAccessFlags _srcAccess,
		VkPipelineStageFlags _dstStage, 
		VkAccessFlags _dstAccess,
		VkImageSubresourceRange _subresourceRange);

	void TransitionImageLayout(VulkanDevice& vkDev,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		uint32_t layerCount = 1,
		uint32_t mipLevels = 1);

	void GenerateMipmap(
		VulkanDevice& vkDev,
		uint32_t maxMipLevels,
		uint32_t width,
		uint32_t height,
		VkImageLayout currentImageLayout
	)
	{
		VkCommandBuffer commandBuffer = vkDev.BeginSingleTimeCommands();

		{
			VkImageSubresourceRange mipbaseRange{};
			mipbaseRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			mipbaseRange.baseMipLevel = 0u;
			mipbaseRange.levelCount = 1u; // The number of mipmap levels (starting from baseMipLevel) accessible to the view
			mipbaseRange.layerCount = layerCount_;

			CreateBarrier(
				commandBuffer, 
				currentImageLayout, 
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT,
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
			CreateBarrier(commandBuffer,
				currentImageLayout, 
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT, 
				VK_ACCESS_TRANSFER_WRITE_BIT,
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
			CreateBarrier(commandBuffer,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // TODO maybe change to currentImageLayout?
				VK_PIPELINE_STAGE_TRANSFER_BIT, 
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT, 
				VK_ACCESS_TRANSFER_READ_BIT,
				mipSubRange);
		}

		vkDev.EndSingleTimeCommands(commandBuffer);
	}

private:
	uint32_t FindMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkFormat FindDepthFormat(VkPhysicalDevice device);
	VkFormat FindSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	bool UpdateTextureImage(
		VulkanDevice& vkDev,
		uint32_t texWidth,
		uint32_t texHeight,
		VkFormat texFormat,
		uint32_t layerCount,
		const void* imageData,
		VkImageLayout sourceImageLayout = VK_IMAGE_LAYOUT_UNDEFINED);

	

	void TransitionImageLayoutCmd(VkCommandBuffer commandBuffer, 
		VkFormat format, 
		VkImageLayout oldLayout, 
		VkImageLayout newLayout, 
		uint32_t layerCount = 1, 
		uint32_t mipLevels = 1);
	bool HasStencilComponent(VkFormat format);

	uint32_t BytesPerTexFormat(VkFormat fmt);
};

#endif