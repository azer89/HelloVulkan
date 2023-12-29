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

	// A reusable sampler which can be accessed by multiple Renderers
	VkSampler defaultImageSampler_;

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
		defaultImageSampler_(nullptr),
		width_(0),
		height_(0),
		mipCount_(0),
		layerCount_(0),
		imageFormat_(VK_FORMAT_UNDEFINED)
	{
	}

	void Destroy(VkDevice device);

	void CreateFromFile(
		VulkanDevice& vkDev,
		const char* filename);

	void CreateAndInitAllObjects(
		VulkanDevice& vkDev,
		const char* filename);

	void CreateFromHDR(
		VulkanDevice& vkDev,
		const char* filename);

	void CreateSampler(
		VkDevice device,
		VkSampler& sampler,
		float minLod = 0.f,
		float maxLod = 0.f,
		VkFilter minFilter = VK_FILTER_LINEAR,
		VkFilter maxFilter = VK_FILTER_LINEAR,
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

	void CreateDefaultSampler(
		VkDevice device,
		float minLod = 0.f,
		float maxLod = 0.f,
		VkFilter minFilter = VK_FILTER_LINEAR,
		VkFilter maxFilter = VK_FILTER_LINEAR,
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

	void CreateImage(
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
		VkImageCreateFlags flags = 0,
		VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);

	void CreateImageFromData(
		VulkanDevice& vkDev,
		void* imageData,
		uint32_t texWidth,
		uint32_t texHeight,
		uint32_t mipmapCount,
		uint32_t layerCount,
		VkFormat texFormat,
		VkImageCreateFlags flags = 0);

	void CreateDepthResources(VulkanDevice& vkDev, uint32_t width, uint32_t height);

	void CreateMultisampledResources(VulkanDevice& vkDev, uint32_t width, uint32_t height);

	void GenerateMipmap(
		VulkanDevice& vkDev,
		uint32_t maxMipLevels,
		uint32_t width,
		uint32_t height,
		VkImageLayout currentImageLayout);

	void CreateImageView(VkDevice device,
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

	// TODO Refactor CreateBarrier() and TransitionImageLayout() to be much simpler
	void CreateBarrier(
		VkCommandBuffer cmdBuffer,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		VkPipelineStageFlags srcStage,
		VkAccessFlags srcAccess,
		VkPipelineStageFlags dstStage,
		VkAccessFlags dstAccess);

	void CreateBarrier(
		VkCommandBuffer cmdBuffer, 
		VkImageLayout oldLayout, 
		VkImageLayout newLayout,
		VkPipelineStageFlags srcStage, 
		VkAccessFlags srcAccess,
		VkPipelineStageFlags dstStage, 
		VkAccessFlags dstAccess,
		VkImageSubresourceRange subresourceRange);

	void TransitionImageLayout(VulkanDevice& vkDev,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		uint32_t layerCount = 1,
		uint32_t mipLevels = 1);

	void TransitionImageLayoutCommand(VkCommandBuffer commandBuffer,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		uint32_t layerCount = 1,
		uint32_t mipLevels = 1);

private:
	uint32_t FindMemoryType(
		VkPhysicalDevice device, 
		uint32_t typeFilter, 
		VkMemoryPropertyFlags properties);
	VkFormat FindDepthFormat(VkPhysicalDevice device);
	VkFormat FindSupportedFormat(
		VkPhysicalDevice device, 
		const std::vector<VkFormat>& candidates, 
		VkImageTiling tiling, 
		VkFormatFeatureFlags features);

	void UpdateImage(
		VulkanDevice& vkDev,
		uint32_t texWidth,
		uint32_t texHeight,
		VkFormat texFormat,
		uint32_t layerCount,
		const void* imageData,
		VkImageLayout sourceImageLayout = VK_IMAGE_LAYOUT_UNDEFINED);

	bool HasStencilComponent(VkFormat format);

	uint32_t BytesPerTexFormat(VkFormat fmt);
};

#endif