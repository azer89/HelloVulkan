#ifndef VULKAN_IMAGE
#define VULKAN_IMAGE

#include "volk.h"
#include "vk_mem_alloc.h"

#include "VulkanDevice.h"

#include <string>

int NumMipMap(int w, int h);

struct ImageBarrierCreateInfo
{
	VkCommandBuffer commandBuffer;
	VkImageLayout oldLayout;
	VkImageLayout newLayout;
	VkPipelineStageFlags sourceStage;
	VkAccessFlags sourceAccess;
	VkPipelineStageFlags destinationStage;
	VkAccessFlags destinationAccess;
};

class VulkanImage
{
public:
	VkImage image_;
	VkImageView imageView_;
	VmaAllocation vmaAllocation_;
	VmaAllocator vmaAllocator_;
	
	// A reusable sampler which can be accessed by multiple Renderers
	VkSampler defaultImageSampler_;

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
		defaultImageSampler_(nullptr),
		width_(0),
		height_(0),
		mipCount_(0),
		layerCount_(0),
		imageFormat_(VK_FORMAT_UNDEFINED),
		multisampleCount_(VK_SAMPLE_COUNT_1_BIT)
	{
	}

	void Destroy(VkDevice device);

	void CreateFromFile(
		VulkanDevice& vkDev,
		const char* filename);

	void CreateImageResources(
		VulkanDevice& vkDev,
		const char* filename);

	void CreateFromHDR(
		VulkanDevice& vkDev,
		const char* filename);

	void CreateSampler(
		VulkanDevice& vkDev,
		VkSampler& sampler,
		float minLod = 0.f,
		float maxLod = 0.f,
		VkFilter minFilter = VK_FILTER_LINEAR,
		VkFilter maxFilter = VK_FILTER_LINEAR,
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

	void CreateDefaultSampler(
		VulkanDevice& vkDev,
		float minLod = 0.f,
		float maxLod = 0.f,
		VkFilter minFilter = VK_FILTER_LINEAR,
		VkFilter maxFilter = VK_FILTER_LINEAR,
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

	void CreateImage(
		VulkanDevice& vkDev,
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
		VulkanDevice& vkDev,
		void* imageData,
		uint32_t texWidth,
		uint32_t texHeight,
		uint32_t mipmapCount,
		uint32_t layerCount,
		VkFormat texFormat,
		VkImageCreateFlags flags = 0);

	// This is used for offscreen rendering as a color attachment
	void CreateColorResources(
		VulkanDevice& vkDev, 
		uint32_t width, 
		uint32_t height,
		VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);

	// Depth attachment for onscreen/offscreen rendering
	void CreateDepthResources(
		VulkanDevice& vkDev, 
		uint32_t width, 
		uint32_t height,
		VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT);

	void GenerateMipmap(
		VulkanDevice& vkDev,
		uint32_t maxMipLevels,
		uint32_t width,
		uint32_t height,
		VkImageLayout currentImageLayout);

	void CreateImageView(
		VulkanDevice& vkDev,
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

	void CreateBarrier(ImageBarrierCreateInfo info);

	void CreateBarrier(ImageBarrierCreateInfo info, VkImageSubresourceRange subresourceRange);

	// TODO Use Facebook IGL
	// github.com/facebook/igl/blob/ac5b27bf6339b022e58d682b590dcf5f00ea2ee8/src/igl/vulkan/VulkanImage.cpp#L638
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

	// To create descriptor sets
	VkDescriptorImageInfo GetDescriptorImageInfo();

	// For validation layer
	void SetDebugName(VulkanDevice& vkDev, const std::string& debugName);

private:
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