#ifndef VULKAN_CONTEXT
#define VULKAN_CONTEXT

#include "VulkanInstance.h"
#include "Configs.h"

// External dependencies
#include "volk.h"
#include "vk_mem_alloc.h"
#include "TracyVulkan.hpp"

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
	VkSemaphore nextSwapchainImageSemaphore_ = nullptr;
	VkSemaphore graphicsQueueSemaphore_ = nullptr;
	VkFence queueSubmitFence_ = nullptr;
	VkCommandBuffer graphicsCommandBuffer_ = nullptr;
	TracyVkCtx tracyContext_ = nullptr;

	void Destroy(VkDevice device)
	{
		vkDestroySemaphore(device, nextSwapchainImageSemaphore_, nullptr);
		vkDestroySemaphore(device, graphicsQueueSemaphore_, nullptr);
		vkDestroyFence(device, queueSubmitFence_, nullptr);
		if (tracyContext_)
		{
			TracyVkDestroy(tracyContext_);
			tracyContext_ = nullptr;
		}
	}
};

struct ContextConfig
{
	bool supportRaytracing_ = false; // If raytracing is enabled then buffer device address is also enabled
	bool suportBufferDeviceAddress_ = false;
	bool supportMSAA_ = true; // TODO This can be disabled but will show a validation error
	bool supportBindlessTextures_ = true;
	bool supportWideLines_ = false;
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
	[[nodiscard]] VkDevice GetDevice() const { return device_; }
	[[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const { return physicalDevice_; }
	[[nodiscard]] VkQueue GetGraphicsQueue() const { return graphicsQueue_; }
	[[nodiscard]] uint32_t GetGraphicsFamily() const { return graphicsFamily_; }
	[[nodiscard]] uint32_t GetComputeFamily() const { return computeFamily_; }
	[[nodiscard]] uint32_t GetSwapchainWidth() const { return swapchainWidth_; }
	[[nodiscard]] uint32_t GetSwapchainHeight() const { return swapchainHeight_; }
	[[nodiscard]] size_t GetDeviceQueueIndicesSize() const { return deviceQueueIndices_.size(); }
	[[nodiscard]] const uint32_t* GetDeviceQueueIndicesData() const { return deviceQueueIndices_.data(); }
	[[nodiscard]] VkSampleCountFlagBits GetMSAASampleCount() const { return msaaSampleCount_; }
	[[nodiscard]] VkQueue GetComputeQueue() const { return computeQueue_; }
	[[nodiscard]] VkFormat GetDepthFormat() const { return depthFormat_; };
	[[nodiscard]] VmaAllocator GetVMAAllocator() const { return vmaAllocator_; }
	[[nodiscard]] bool SupportBufferDeviceAddress() const { return config_.supportRaytracing_ || config_.suportBufferDeviceAddress_; }

	// Getters related to swapchain
	[[nodiscard]] VkSwapchainKHR GetSwapChain() const { return swapchain_; }
	[[nodiscard]] size_t GetSwapchainImageCount() const { return swapchainImages_.size(); }
	[[nodiscard]] VkFormat GetSwapchainImageFormat() const { return swapchainImageFormat_; }
	[[nodiscard]] VkImage GetSwapchainImage(size_t i) const { return swapchainImages_[i]; }
	[[nodiscard]] VkImageView GetSwapchainImageView(size_t i) const { return swapchainImageViews_[i]; }
	[[nodiscard]] uint32_t GetCurrentSwapchainImageIndex() const { return currentSwapchainImageIndex_; }

	// Raytracing getters
	[[nodiscard]] VkPhysicalDeviceRayTracingPipelinePropertiesKHR GetRayTracingPipelineProperties() const { return rtPipelineProperties_; }

	// Pointer getters
	[[nodiscard]] VkSwapchainKHR* GetSwapchainPtr() { return &swapchain_; }

	// Sync objects and render command buffer
	void IncrementFrameIndex();
	[[nodiscard]] FrameData& GetCurrentFrameData();
	[[nodiscard]] uint32_t GetFrameIndex() const;
	[[nodiscard]] TracyVkCtx GetTracyContext() const { return frameDataArray_[GetFrameIndex()].tracyContext_; }

	// Debugging
	void SetVkObjectName(void* objectHandle, VkObjectType objType, const char* name) const;
	void InsertDebugLabel(VkCommandBuffer commandBuffer, const char* label, uint32_t colorRGBA) const;

private:
	void CreateDevice();
	VkResult CreatePhysicalDevice(VkInstance instance);
	uint32_t FindQueueFamilies(VkQueueFlags desiredFlags);
	bool IsDeviceSuitable(VkPhysicalDevice d);
	void CheckSurfaceSupport(VulkanInstance& instance);
	VkSampleCountFlagBits GetMaxUsableSampleCount(VkPhysicalDevice d);

	// Swapchain
	VkResult CreateSwapchain(VkSurfaceKHR surface);
	size_t CreateSwapchainImages();
	void CreateSwapChainImageView(
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
	VkSwapchainKHR swapchain_ = nullptr;
	std::vector<VkImage> swapchainImages_ = {};
	std::vector<VkImageView> swapchainImageViews_ = {};
	VkFormat swapchainImageFormat_ = VK_FORMAT_UNDEFINED;
	uint32_t currentSwapchainImageIndex_ = 0; // Current image index
	uint32_t swapchainWidth_ = 0;
	uint32_t swapchainHeight_ = 0;
	
	VkFormat depthFormat_ = VK_FORMAT_UNDEFINED;
	VkSampleCountFlagBits msaaSampleCount_ = VK_SAMPLE_COUNT_1_BIT;

	VkDevice device_ = nullptr;
	VkPhysicalDevice physicalDevice_ = nullptr;;
	VkPhysicalDeviceMemoryProperties memoryProperties_ = {};

	// Graphics
	uint32_t graphicsFamily_ = 0;
	VkQueue graphicsQueue_ = nullptr;
	VkCommandPool graphicsCommandPool_ = nullptr;

	// Compute
	uint32_t computeFamily_ = 0;
	VkQueue computeQueue_ = nullptr;
	VkCommandPool computeCommandPool_ = nullptr;

	std::vector<uint32_t> deviceQueueIndices_ = {};

	uint32_t frameIndex_ = 0;
	std::array<FrameData, AppConfig::FrameCount> frameDataArray_ = {};

	VmaAllocator vmaAllocator_ = nullptr;

	ContextConfig config_ = {};

	// Raytracing and bindless
	VkPhysicalDeviceBufferDeviceAddressFeatures deviceAddressEnabledFeatures_ = {};

	// Raytracing
	VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtPipelineProperties_ = {};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR rtASFeatures_ = {};
	
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineEnabledFeatures_ = {};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR rtASEnabledFeatures = {};

	// Bindless Textures
	VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingFeatures_ = {};

	// Features
	// NOTE features2_ and features13_ are always created
	VkPhysicalDeviceVulkan13Features features13_ = {};
	VkPhysicalDeviceVulkan11Features features11_ = {};
	VkPhysicalDeviceFeatures2 features2_ = {};
	VkPhysicalDeviceFeatures features_ = {};
};

#endif
