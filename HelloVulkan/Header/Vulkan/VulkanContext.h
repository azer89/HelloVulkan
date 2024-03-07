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

	bool supportBindlessRendering_ = true;

	// TODO Set validation layer as optional
};

/*
Class that encapsulate a Vulkan device, the swapchain, and a VMA allocator.
*/
class VulkanContext
{
public:
	VulkanContext() = default;
	~VulkanContext() = default;

	// Not copyable or movable
	VulkanContext(const VulkanContext&) = delete;
	VulkanContext& operator=(const VulkanContext&) = delete;
	VulkanContext(VulkanContext&&) = delete;
	VulkanContext& operator=(VulkanContext&&) = delete;

	void Create(
		VulkanInstance& instance,
		ContextConfig config
	);
	
	void Destroy();

	void RecreateSwapchainResources(
		VulkanInstance& instance,
		uint32_t width,
		uint32_t height);
	VkResult GetNextSwapchainImage(VkSemaphore nextSwapchainImageSemaphore);

	[[nodiscard]] VkCommandBuffer BeginOneTimeGraphicsCommand() const;
	void EndOneTimeGraphicsCommand(VkCommandBuffer commandBuffer) const;
	[[nodiscard]] VkCommandBuffer BeginOneTimeComputeCommand() const;
	void EndOneTimeComputeCommand(VkCommandBuffer commandBuffer) const;

	// Getters
	VkDevice GetDevice() const { return device_; }
	VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice_; }
	VkQueue GetGraphicsQueue() const { return graphicsQueue_; }
	uint32_t GetGraphicsFamily() const { return graphicsFamily_; }
	uint32_t GetComputeFamily() const { return computeFamily_; }
	uint32_t GetSwapchainWidth() const { return swapchainWidth_; }
	uint32_t GetSwapchainHeight() const { return swapchainHeight_; }
	size_t GetDeviceQueueIndicesSize() const { return deviceQueueIndices_.size(); }
	const uint32_t* GetDeviceQueueIndicesData() const { return deviceQueueIndices_.data(); }
	VkSampleCountFlagBits GetMSAASampleCount() const { return msaaSampleCount_; }
	VkQueue GetComputeQueue() const { return computeQueue_; }
	VkFormat GetDepthFormat() const { return depthFormat_; };
	VmaAllocator GetVMAAllocator() const { return vmaAllocator_; }

	// Raytracing getters
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR GetRayTracingPipelineProperties() const { return rtPipelineProperties_; }

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
	void SetVkObjectName(void* objectHandle, VkObjectType objType, const char* name) const;

private:
	VkResult CreatePhysicalDevice(VkInstance instance);
	uint32_t FindQueueFamilies(VkQueueFlags desiredFlags);
	VkResult CreateDevice();
	bool IsDeviceSuitable(VkPhysicalDevice d);
	void CheckSurfaceSupport(VulkanInstance& instance);
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

	VkResult CreateSemaphore(VkSemaphore* outSemaphore) const;
	VkResult CreateFence(VkFence* fence) const;
	VkResult CreateCommandBuffer(VkCommandPool pool, VkCommandBuffer* commandBuffer) const;
	VkResult CreateCommandPool(uint32_t family, VkCommandPool* pool) const;

	void GetQueues();
	void AllocateFrameInFlightData();
	void AllocateVMA(VulkanInstance& instance);

	void GetRaytracingPropertiesAndFeatures();
	void ChainFeatures();

	VkFormat FindDepthFormat() const;
	VkFormat FindSupportedFormat(
		const std::vector<VkFormat>& candidates,
		VkImageTiling tiling,
		VkFormatFeatureFlags features) const;

private:
	VkSwapchainKHR swapchain_;
	std::vector<VkImage> swapchainImages_;
	std::vector<VkImageView> swapchainImageViews_;
	VkFormat swapchainImageFormat_;
	uint32_t currentSwapchainImageIndex_; // Current image index
	uint32_t swapchainWidth_;
	uint32_t swapchainHeight_;
	
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

	// Bindless Textures
	VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingFeatures_;
	//VkPhysicalDeviceShaderDrawParametersFeatures shaderDrawFeatures_;

	// Features
	// NOTE features2_ and features13_ are always created
	VkPhysicalDeviceVulkan13Features features13_;
	VkPhysicalDeviceVulkan11Features features11_;
	VkPhysicalDeviceFeatures2 features2_;
	VkPhysicalDeviceFeatures features_;
};

#endif
