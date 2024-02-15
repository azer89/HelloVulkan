#ifndef VULKAN_CONTEXT
#define VULKAN_CONTEXT

#include "VulkanInstance.h"
#include "Configs.h"

#include "volk.h"
#include "vk_mem_alloc.h"

#include <vector>
#include <array>

struct SwapchainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities_ = {};
	std::vector<VkSurfaceFormatKHR> formats_;
	std::vector<VkPresentModeKHR> presentModes_;
};

/*
Struct containing objects needed for every frame draw 
*/
struct FrameData
{
	VkSemaphore nextSwapchainImageSemaphore_;
	VkSemaphore graphicsQueueSemaphore_;
	VkFence queueSubmitFence_;
	VkCommandBuffer graphicsCommandBuffer_;

	void Destroy(VkDevice device)
	{
		vkDestroySemaphore(device, nextSwapchainImageSemaphore_, nullptr);
		vkDestroySemaphore(device, graphicsQueueSemaphore_, nullptr);
		vkDestroyFence(device, queueSubmitFence_, nullptr);
	}
};

struct ContextConfig
{
	bool supportRaytracing_ = false;
	
	// This can be disabled but validation layer will complain a little bit
	bool supportMSAA_ = true;

	// TODO Set validation layer as optional
};

/*
Class that encapsulate a Vulkan device, the swapchain, and a VMA allocator.
Maybe this should be renamed to VulkanContext.
*/
class VulkanDevice
{
public:
	VulkanDevice() = default;
	~VulkanDevice() = default;

	void Create(
		VulkanInstance& instance,
		ContextConfig config
	);

	void CheckSurfaceSupport(VulkanInstance& instance);

	void Destroy();

	void RecreateSwapchainResources(
		VulkanInstance& instance,
		uint32_t width,
		uint32_t height);
	VkResult GetNextSwapchainImage(VkSemaphore nextSwapchainImageSemaphore);

	// TODO Maybe these four functions can be simplified/combined
	VkCommandBuffer BeginOneTimeGraphicsCommand();
	void EndOneTimeGraphicsCommand(VkCommandBuffer commandBuffer);
	VkCommandBuffer BeginOneTimeComputeCommand();
	void EndOneTimeComputeCommand(VkCommandBuffer commandBuffer);

	// Getters
	VkDevice GetDevice() const { return device_; }
	VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice_; }
	VkQueue GetGraphicsQueue() const { return graphicsQueue_; }
	uint32_t GetGraphicsFamily() const { return graphicsFamily_; }
	uint32_t GetComputeFamily() const { return computeFamily_; }
	uint32_t GetFrameBufferWidth() const { return framebufferWidth_; }
	uint32_t GetFrameBufferHeight() const { return framebufferHeight_; }
	size_t GetDeviceQueueIndicesSize() const { return deviceQueueIndices_.size(); }
	const uint32_t* GetDeviceQueueIndicesData() const { return deviceQueueIndices_.data(); }
	VkSampleCountFlagBits GetMSAASampleCount() const { return msaaSampleCount_; }
	VkQueue GetComputeQueue() const { return computeQueue_; }
	VkFormat GetDepthFormat() const { return depthFormat_; };
	VmaAllocator GetVMAAllocator() const { return vmaAllocator_; }

	// Raytracing getters
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR GetRayTracingPipelineProperties() { return rtPipelineProperties_; }
	VkPhysicalDeviceAccelerationStructureFeaturesKHR GetAccelerationStructureFeatures() { rtASFeatures_; }

	// Getters related to swapchain
	VkSwapchainKHR GetSwapChain() const { return swapchain_; }
	size_t GetSwapchainImageCount() const { return swapchainImages_.size(); }
	VkFormat GetSwapchainImageFormat() const { return swapchainImageFormat_; }
	VkImage GetSwapchainImage(size_t i) const { return swapchainImages_[i]; }
	VkImageView GetSwapchainImageView(size_t i) const { return swapchainImageViews_[i]; }
	uint32_t GetCurrentSwapchainImageIndex() const { return currentSwapchainImageIndex_; }

	// Pointer getters
	VkSwapchainKHR* GetSwapchainPtr() { return &swapchain_; }

	// Sync objects and render command buffer
	FrameData& GetCurrentFrameData();
	void IncrementFrameIndex();
	uint32_t GetFrameIndex() const;

	// For debugging purpose
	void SetVkObjectName(void* objectHandle, VkObjectType objType, const char* name);

private:
	VkResult CreatePhysicalDevice(VkInstance instance);
	uint32_t FindQueueFamilies(VkQueueFlags desiredFlags);
	VkResult CreateDevice();
	bool IsDeviceSuitable(VkPhysicalDevice d);
	VkSampleCountFlagBits GetMaxUsableSampleCount(VkPhysicalDevice d);

	// Swapchain
	VkResult CreateSwapchain(VkSurfaceKHR surface);
	size_t CreateSwapchainImages();
	bool CreateSwapChainImageView(
		size_t imageIndex,
		VkFormat format, 
		VkImageAspectFlags aspectFlags);
	SwapchainSupportDetails QuerySwapchainSupport(VkSurfaceKHR surface);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	uint32_t GetSwapchainImageCount(const VkSurfaceCapabilitiesKHR& capabilities_);

	VkResult CreateSemaphore(VkSemaphore* outSemaphore);
	VkResult CreateFence(VkFence* fence);
	VkResult CreateCommandBuffer(VkCommandPool pool, VkCommandBuffer* commandBuffer);
	VkResult CreateCommandPool(uint32_t family, VkCommandPool* pool);

	void GetQueues();
	void AllocateFrameInFlightData();
	void AllocateVMA(VulkanInstance& instance);

	void GetRaytracingPropertiesAndFeatures();
	void GetEnabledRaytracingFeatures();

	VkFormat FindDepthFormat();
	VkFormat FindSupportedFormat(
		const std::vector<VkFormat>& candidates,
		VkImageTiling tiling,
		VkFormatFeatureFlags features);

private:
	VkSwapchainKHR swapchain_;
	std::vector<VkImage> swapchainImages_;
	std::vector<VkImageView> swapchainImageViews_;
	VkFormat swapchainImageFormat_;
	uint32_t currentSwapchainImageIndex_; // Current image index

	uint32_t framebufferWidth_;
	uint32_t framebufferHeight_;
	VkFormat depthFormat_;
	VkSampleCountFlagBits msaaSampleCount_;

	VkDevice device_;
	VkPhysicalDevice physicalDevice_;
	VkPhysicalDeviceMemoryProperties memoryProperties_;

	// Graphics
	VkQueue graphicsQueue_;
	uint32_t graphicsFamily_;
	// Note that all graphics command buffers are created from this command pool below.
	// So you need to create multiple command pools if you want to use vkResetCommandPool().
	VkCommandPool graphicsCommandPool_;

	// Compute
	VkQueue computeQueue_;
	uint32_t computeFamily_;
	VkCommandPool computeCommandPool_;

	std::vector<uint32_t> deviceQueueIndices_;

	uint32_t frameIndex_;
	std::array<FrameData, AppConfig::FrameOverlapCount> frameDataArray_;

	VmaAllocator vmaAllocator_;

	ContextConfig config_;

	// Raytracing
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtPipelineProperties_;
	VkPhysicalDeviceAccelerationStructureFeaturesKHR rtASFeatures_;

	VkPhysicalDeviceBufferDeviceAddressFeatures rtDevAddressEnabledFeatures_;
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineEnabledFeatures_;
	VkPhysicalDeviceAccelerationStructureFeaturesKHR rtASEnabledFeatures;

	// pNext structure for passing extension structures to device creation
	void* pNextChain_ = nullptr;
};

#endif
