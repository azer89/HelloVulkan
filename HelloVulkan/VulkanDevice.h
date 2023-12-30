#ifndef VULKAN_DEVICE
#define VULKAN_DEVICE

#include "VulkanInstance.h"

#include "volk.h"

#include <vector>

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities = {};
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class VulkanDevice
{
public:
	void CreateCompute(
		VulkanInstance& instance,
		uint32_t width, 
		uint32_t height, 
		VkPhysicalDeviceFeatures deviceFeatures
	);

	void Destroy();

	VkDevice GetDevice() const { return device_; }
	VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice_; }
	VkSwapchainKHR GetSwapChain() const { return swapchain_; }
	VkSemaphore GetSemaphore() const { return semaphore_; }
	VkCommandPool GetCommandPool() const { return commandPool_; }
	VkQueue GetGraphicsQueue() const { return graphicsQueue_; }
	uint32_t GetFrameBufferWidth() const { return framebufferWidth_; }
	uint32_t GetFrameBufferHeight() const { return framebufferHeight_; }
	size_t GetSwapChainImageSize() const { return swapchainImages_.size(); }
	size_t GetDeviceQueueIndicesSize() const { return deviceQueueIndices_.size(); }
	const uint32_t* GetDeviceQueueIndicesData() const { return deviceQueueIndices_.data(); }
	VkCommandBuffer GetComputeCommandBuffer() { return computeCommandBuffer_; }
	VkQueue GetComputeQueue() { return computeQueue_; }

	VkImageView GetSwapchainImageView(unsigned i)
	{
		return swapchainImageViews_[i];
	}

	VkFormat FindDepthFormat();

	VkFormat FindSupportedFormat(
		const std::vector<VkFormat>& candidates,
		VkImageTiling tiling,
		VkFormatFeatureFlags features);

	VkCommandBuffer BeginSingleTimeCommands();

	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

private:
	VkResult CreatePhysicalDevice(VkInstance instance);
	uint32_t FindQueueFamilies(VkQueueFlags desiredFlags);
	VkResult CreateDeviceWithCompute(
		VkPhysicalDeviceFeatures deviceFeatures, 
		uint32_t graphicsFamily, 
		uint32_t computeFamily);
	VkResult CreateDevice(
		VkPhysicalDeviceFeatures deviceFeatures, 
		uint32_t graphicsFamily);
	bool IsDeviceSuitable(VkPhysicalDevice d);

	// Swap chain
	VkResult CreateSwapchain(VkSurfaceKHR surface, bool supportScreenshots = false);
	size_t CreateSwapchainImages();
	bool CreateSwapChainImageView(
		unsigned imageIndex,
		VkFormat format, 
		VkImageAspectFlags aspectFlags,
		VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, 
		uint32_t layerCount = 1, 
		uint32_t mipLevels = 1);
	SwapchainSupportDetails QuerySwapchainSupport(VkSurfaceKHR surface);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	uint32_t ChooseSwapImageCount(const VkSurfaceCapabilitiesKHR& capabilities);

	// Sync
	VkResult CreateSemaphore(VkSemaphore* outSemaphore);

public:
	VkSwapchainKHR swapchain_;
	VkSemaphore semaphore_;
	VkSemaphore renderSemaphore_;
	std::vector<VkCommandBuffer> commandBuffers_;

private:
	// A queue of rendered images waiting to be presented to the screen
	std::vector<VkImage> swapchainImages_;
	std::vector<VkImageView> swapchainImageViews_;
	VkFormat swapchainImageFormat_;

	uint32_t framebufferWidth_;
	uint32_t framebufferHeight_;

	VkDevice device_;
	VkPhysicalDevice physicalDevice_;
	VkQueue graphicsQueue_;

	uint32_t graphicsFamily_;

	VkCommandPool commandPool_;

	// Were we initialized with compute capabilities
	bool useCompute_ = false;

	// [may coincide with graphicsFamily]
	uint32_t computeFamily_;
	VkQueue computeQueue_;

	// A list of all queues (for shared buffer allocation)
	std::vector<uint32_t> deviceQueueIndices_;
	std::vector<VkQueue> deviceQueues_;

	VkCommandBuffer computeCommandBuffer_;
	VkCommandPool computeCommandPool_;
};

#endif
