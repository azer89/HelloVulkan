#ifndef VULKAN_IMAGE
#define VULKAN_IMAGE

#include "VulkanContext.h"

#include <string>

class VulkanImage
{
public:
	VkImage image_;
	VkImageView imageView_;
	VmaAllocation vmaAllocation_ ;

	// A reusable sampler which can be accessed by multiple Renderers
	VkSampler defaultImageSampler_;

	// Cached for Destroy()
	VkDevice device_;
	VmaAllocator vmaAllocator_;

	uint32_t width_;
	uint32_t height_;
	uint32_t mipCount_;
	uint32_t layerCount_;
	VkFormat imageFormat_;
	VkSampleCountFlagBits multisampleCount_;

public:
	VulkanImage() :
		image_(nullptr),
		imageView_(nullptr),
		vmaAllocation_(nullptr),
		vmaAllocator_(nullptr),
		device_(nullptr),
		defaultImageSampler_(nullptr),
		width_(0),
		height_(0),
		mipCount_(0),
		layerCount_(0),
		imageFormat_(VK_FORMAT_UNDEFINED),
		multisampleCount_(VK_SAMPLE_COUNT_1_BIT)
	{
	}

	void Destroy();

	void CreateFromFile(
		VulkanContext& ctx,
		const char* filename);

	// Create a mipmapped image, an image view, and a sampler
	void CreateImageResources(
		VulkanContext& ctx,
		const char* filename);

	// Create a mipmapped image, an image view, and a sampler
	void CreateImageResources(
		VulkanContext& ctx,
		void* data,
		int width,
		int height);

	void CreateFromHDR(
		VulkanContext& ctx,
		const char* filename);

	void CreateSampler(
		VulkanContext& ctx,
		VkSampler& sampler,
		float minLod = 0.f,
		float maxLod = 0.f,
		VkFilter minFilter = VK_FILTER_LINEAR,
		VkFilter maxFilter = VK_FILTER_LINEAR,
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

	void CreateDefaultSampler(
		VulkanContext& ctx,
		float minLod = 0.f,
		float maxLod = 0.f,
		VkFilter minFilter = VK_FILTER_LINEAR,
		VkFilter maxFilter = VK_FILTER_LINEAR,
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

	void CreateImage(
		VulkanContext& ctx,
		uint32_t width,
		uint32_t height,
		uint32_t mipCount,
		uint32_t layerCount,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags imageUsage,
		VmaMemoryUsage memoryUsage,
		VkImageCreateFlags flags = 0,
		VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);

	void CreateImageFromData(
		VulkanContext& ctx,
		void* imageData,
		uint32_t texWidth,
		uint32_t texHeight,
		uint32_t mipmapCount,
		uint32_t layerCount,
		VkFormat texFormat,
		VkImageCreateFlags flags = 0);

	// This is used for offscreen rendering as a color attachment
	void CreateColorResources(
		VulkanContext& ctx, 
		uint32_t width, 
		uint32_t height,
		VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);

	// Depth attachment for onscreen/offscreen rendering
	void CreateDepthResources(
		VulkanContext& ctx, 
		uint32_t width, 
		uint32_t height,
		uint32_t layerCount,
		VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT,
		VkImageUsageFlags additionalUsage = 0);

	void GenerateMipmap(
		VulkanContext& ctx,
		uint32_t maxMipLevels,
		uint32_t width,
		uint32_t height,
		VkImageLayout currentImageLayout);

	void CreateImageView(
		VulkanContext& ctx,
		VkFormat format,
		VkImageAspectFlags aspectFlags,
		VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D,
		uint32_t mipLevel = 0u,
		uint32_t mipCount = 1u,
		uint32_t layerLevel = 0u,
		uint32_t layerCount = 1u);

	static void CreateImageView(
		VulkanContext& ctx,
		VkImage image,
		VkImageView& view,
		VkFormat format,
		VkImageAspectFlags aspectFlags,
		VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D,
		uint32_t mipLevel = 0u,
		uint32_t mipCount = 1u,
		uint32_t layerLevel = 0u,
		uint32_t layerCount = 1u);


	void CopyBufferToImage(
		VulkanContext& ctx,
		VkBuffer buffer,
		uint32_t width,
		uint32_t height,
		uint32_t layerCount = 1);

	// This transitions all mip levels and all layers
	void TransitionLayout(
		VulkanContext& ctx,
		VkImageLayout oldLayout,
		VkImageLayout newLayout);
	
	void TransitionLayout(
		VulkanContext& ctx,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		// By default, this transitions one mip level and one layers
		uint32_t mipLevel = 0u,
		uint32_t mipCount = 1u,
		uint32_t layerLevel = 0u,
		uint32_t layerCount = 1u
	);

	static void TransitionLayoutCommand(
		VkCommandBuffer commandBuffer,
		VkImage image,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		// By default, this transitions one mip level and one layers
		uint32_t mipLevel = 0u,
		uint32_t mipCount = 1u,
		uint32_t layerLevel = 0u,
		uint32_t layerCount = 1u
	);

	// To create descriptor sets
	VkDescriptorImageInfo GetDescriptorImageInfo() const;

	// For validation layer
	void SetDebugName(VulkanContext& ctx, const std::string& debugName);

private:
	void UpdateImage(
		VulkanContext& ctx,
		uint32_t texWidth,
		uint32_t texHeight,
		VkFormat texFormat,
		uint32_t layerCount,
		const void* imageData,
		VkImageLayout sourceImageLayout = VK_IMAGE_LAYOUT_UNDEFINED);

	uint32_t BytesPerTexFormat(VkFormat fmt);
};

#endif