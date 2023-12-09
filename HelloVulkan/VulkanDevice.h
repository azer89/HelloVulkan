#ifndef VULKAN_DEVICE
#define VULKAN_DEVICE

#include "VulkanInstance.h"

#define VK_NO_PROTOTYPES
#include "volk.h"

#include <vector>
#include <functional>

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

	VkDevice GetDevice() { return device; }
	VkPhysicalDevice GetPhysicalDevice() { return physicalDevice; }
	VkCommandPool GetCommandPool() { return commandPool; }
	VkQueue GetGraphicsQueue() { return graphicsQueue; }

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

private:
	uint32_t framebufferWidth;
	uint32_t framebufferHeight;

	VkDevice device;
	VkPhysicalDevice physicalDevice;
	VkQueue graphicsQueue;

	uint32_t graphicsFamily;

	VkSwapchainKHR swapchain;
	VkSemaphore semaphore;
	VkSemaphore renderSemaphore;

	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	// For chapter5/6etc (compute shaders)

	// Were we initialized with compute capabilities
	bool useCompute = false;

	// [may coincide with graphicsFamily]
	uint32_t computeFamily;
	VkQueue computeQueue;

	// a list of all queues (for shared buffer allocation)
	std::vector<uint32_t> deviceQueueIndices;
	std::vector<VkQueue> deviceQueues;

	VkCommandBuffer computeCommandBuffer;
	VkCommandPool computeCommandPool;
};

#endif
