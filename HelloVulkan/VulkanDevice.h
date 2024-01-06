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

	void RecreateSwapchainResources(
		VulkanInstance& instance,
		uint32_t width,
		uint32_t height);

	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

	// Getters
	VkDevice GetDevice() const { return device_; }
	VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice_; }
	VkCommandPool GetCommandPool() const { return commandPool_; }
	VkQueue GetGraphicsQueue() const { return graphicsQueue_; }
	uint32_t GetFrameBufferWidth() const { return framebufferWidth_; }
	uint32_t GetFrameBufferHeight() const { return framebufferHeight_; }
	size_t GetDeviceQueueIndicesSize() const { return deviceQueueIndices_.size(); }
	const uint32_t* GetDeviceQueueIndicesData() const { return deviceQueueIndices_.data(); }
	VkSampleCountFlagBits GetMSAASampleCount() const { return msaaSampleCount_; }
	VkCommandBuffer GetComputeCommandBuffer() const { return computeCommandBuffer_; }
	VkQueue GetComputeQueue() const { return computeQueue_; }
	VkFormat GetDepthFormat() const { return depthFormat_; };
	VkCommandBuffer GetCommandBuffer(unsigned int index) const;

	// Getters related to swapchain
	VkSwapchainKHR GetSwapChain() const { return swapchain_; }
	size_t GetSwapchainImageCount() const { return swapchainImages_.size(); }
	VkImageView GetSwapchainImageView(unsigned i) const { return swapchainImageViews_[i]; }
	VkFormat GetSwaphchainImageFormat() const { return swapchainImageFormat_; }

	// Pointer getters
	VkSwapchainKHR* GetSwapchainPtr() { return &swapchain_; }
	VkSemaphore* GetSwapchainSemaphorePtr() { return &swapchainSemaphore_; }
	VkCommandBuffer* GetCommandBufferPtr(unsigned int index);
	VkSemaphore* GetRenderSemaphorePtr() { return &renderSemaphore_; }

	// For debugging purpose
	void SetVkObjectName(void* objectHandle, VkObjectType objType, const char* name);

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
	VkSampleCountFlagBits GetMaxUsableSampleCount(VkPhysicalDevice d);

	// Swap chain
	VkResult CreateSwapchain(VkSurfaceKHR surface);
	size_t CreateSwapchainImages();
	bool CreateSwapChainImageView(
		unsigned imageIndex,
		VkFormat format, 
		VkImageAspectFlags aspectFlags);
	SwapchainSupportDetails QuerySwapchainSupport(VkSurfaceKHR surface);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	uint32_t GetSwapchainImageCount(const VkSurfaceCapabilitiesKHR& capabilities);

	// Sync
	VkResult CreateSemaphore(VkSemaphore* outSemaphore);

	VkFormat FindDepthFormat();

	VkFormat FindSupportedFormat(
		const std::vector<VkFormat>& candidates,
		VkImageTiling tiling,
		VkFormatFeatureFlags features);

private:
	// Sync
	VkSemaphore swapchainSemaphore_;
	VkSemaphore renderSemaphore_;

	VkSwapchainKHR swapchain_;
	// A queue of rendered images waiting to be presented to the screen
	std::vector<VkImage> swapchainImages_;
	std::vector<VkImageView> swapchainImageViews_;
	VkFormat swapchainImageFormat_;
	// We have one command buffer per swapchain
	std::vector<VkCommandBuffer> swapchainCommandBuffers_;

	uint32_t framebufferWidth_;
	uint32_t framebufferHeight_;
	VkFormat depthFormat_;
	VkSampleCountFlagBits msaaSampleCount_;

	VkDevice device_;
	VkPhysicalDevice physicalDevice_;

	VkQueue graphicsQueue_;
	uint32_t graphicsFamily_;
	VkCommandPool commandPool_;

	// This may coincide with graphicsFamily
	VkQueue computeQueue_;
	uint32_t computeFamily_;
	bool useCompute_ = false;
	VkCommandBuffer computeCommandBuffer_;
	VkCommandPool computeCommandPool_;

	std::vector<uint32_t> deviceQueueIndices_;
};

#endif
