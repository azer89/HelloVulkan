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

	VkDevice GetDevice() const { return device; }
	VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice; }
	VkSwapchainKHR GetSwapChain() const { return swapchain; }
	VkSemaphore GetSemaphore() const { return semaphore; }
	VkCommandPool GetCommandPool() const { return commandPool; }
	VkQueue GetGraphicsQueue() const { return graphicsQueue; }
	uint32_t GetFrameBufferWidth() const { return framebufferWidth; }
	uint32_t GetFrameBufferHeight() const { return framebufferHeight; }
	size_t GetSwapChainImageSize() const { return swapchainImages.size(); }
	size_t GetDeviceQueueIndicesSize() const { return deviceQueueIndices.size(); }
	const uint32_t* GetDeviceQueueIndicesData() const { return deviceQueueIndices.data(); }
	VkCommandBuffer GetComputeCommandBuffer() { return computeCommandBuffer; }
	VkQueue GetComputeQueue() { return computeQueue; }

	VkImageView GetSwapchainImageView(unsigned i)
	{
		return swapchainImageViews[i];
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
	bool CreateImageView(
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
	VkSwapchainKHR swapchain;
	VkSemaphore semaphore;
	VkSemaphore renderSemaphore;
	std::vector<VkCommandBuffer> commandBuffers;

private:
	uint32_t framebufferWidth;
	uint32_t framebufferHeight;

	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkQueue graphicsQueue;

	uint32_t graphicsFamily;

	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;

	VkCommandPool commandPool;

	// Were we initialized with compute capabilities
	bool useCompute = false;

	// [may coincide with graphicsFamily]
	uint32_t computeFamily;
	VkQueue computeQueue;

	// A list of all queues (for shared buffer allocation)
	std::vector<uint32_t> deviceQueueIndices;
	std::vector<VkQueue> deviceQueues;

	VkCommandBuffer computeCommandBuffer;
	VkCommandPool computeCommandPool;
};

#endif
