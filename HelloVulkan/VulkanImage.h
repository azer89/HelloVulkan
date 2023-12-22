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

	void GenerateMipmap(
		VulkanDevice& vkDev,
		uint32_t maxMipLevels,
		uint32_t width,
		uint32_t height,
		VkImageLayout currentImageLayout);

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