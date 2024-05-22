#include "VulkanContext.h"
#include "VulkanCheck.h"
#include "Configs.h"

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#include "vk_mem_alloc.h"

#include <iostream>

void VulkanContext::Create(VulkanInstance& instance, ContextConfig config)
{
	config_ = config;
	swapchainWidth_ = AppConfig::InitialScreenWidth;
	swapchainHeight_ = AppConfig::InitialScreenHeight;

	// Enable features
	ChainFeatures();

	VK_CHECK(CreatePhysicalDevice(instance.GetInstance()));
	graphicsFamily_ = FindQueueFamilies(VK_QUEUE_GRAPHICS_BIT);
	computeFamily_ = FindQueueFamilies(VK_QUEUE_COMPUTE_BIT);
	CreateDevice();

	GetRaytracingPropertiesAndFeatures();

	GetQueues();

	CheckSurfaceSupport(instance);

	// Swapchain
	VK_CHECK(CreateSwapchain(instance.GetSurface()));
	CreateSwapchainImages();

	// Command pools
	VK_CHECK(CreateCommandPool(graphicsFamily_, &graphicsCommandPool_));
	VK_CHECK(CreateCommandPool(computeFamily_, &computeCommandPool_));

	// Frame in flight
	AllocateFrameInFlightData();

	// VMA
	AllocateVMA(instance);
}

void VulkanContext::Destroy()
{
	for (uint32_t i = 0; i < AppConfig::FrameCount; ++i)
	{
		frameDataArray_[i].Destroy(device_);
	}
	for (size_t i = 0; i < swapchainImages_.size(); i++)
	{
		vkDestroyImageView(device_, swapchainImageViews_[i], nullptr);
	}
	vkDestroySwapchainKHR(device_, swapchain_, nullptr);
	vkDestroyCommandPool(device_, graphicsCommandPool_, nullptr);
	vkDestroyCommandPool(device_, computeCommandPool_, nullptr);
	vmaDestroyAllocator(vmaAllocator_);
	vkDestroyDevice(device_, nullptr);
}

void VulkanContext::GetQueues()
{
	deviceQueueIndices_.push_back(graphicsFamily_);
	if (graphicsFamily_ != computeFamily_)
	{
		deviceQueueIndices_.push_back(computeFamily_);
	}

	vkGetDeviceQueue(device_, graphicsFamily_, 0, &graphicsQueue_);
	if (graphicsQueue_ == nullptr) { throw std::runtime_error("Cannot obtain graphics queue"); }
	
	vkGetDeviceQueue(device_, computeFamily_, 0, &computeQueue_);
	if (computeQueue_ == nullptr) { throw std::runtime_error("Cannot obtain compute queue"); }
}

void VulkanContext::AllocateVMA(VulkanInstance& instance)
{
	// Need this because we use Volk
	const VmaVulkanFunctions vulkanFunctions =
	{
		.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
		.vkGetDeviceProcAddr = vkGetDeviceProcAddr,
	};

	VmaAllocatorCreateFlags flags = 0u;
	if (config_.supportRaytracing_ || config_.suportBufferDeviceAddress_)
	{
		// Activate buffer address
		flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	}

	const VmaAllocatorCreateInfo allocatorInfo =
	{
		.flags = flags,
		.physicalDevice = physicalDevice_,
		.device = device_,
		.pVulkanFunctions = (const VmaVulkanFunctions*)&vulkanFunctions,
		.instance = instance.GetInstance(),
	};

	vmaCreateAllocator(&allocatorInfo, &vmaAllocator_);
}

void VulkanContext::CheckSurfaceSupport(VulkanInstance& instance) const
{
	VkBool32 presentSupported = 0;
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice_, graphicsFamily_, instance.GetSurface(), &presentSupported);
	if (!presentSupported) { throw std::runtime_error("Cannot obtain surface support KHR"); }
}

void VulkanContext::GetRaytracingPropertiesAndFeatures()
{
	if (config_.supportRaytracing_)
	{
		// Properties
		rtPipelineProperties_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
		VkPhysicalDeviceProperties2 deviceProperties2 =
		{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
			.pNext = &rtPipelineProperties_
		};
		vkGetPhysicalDeviceProperties2(physicalDevice_, &deviceProperties2);

		// Features
		rtASFeatures_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
		VkPhysicalDeviceFeatures2 deviceFeatures2 =
		{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
			.pNext = &rtASFeatures_
		};
		vkGetPhysicalDeviceFeatures2(physicalDevice_, &deviceFeatures2);
	}
}

void VulkanContext::ChainFeatures()
{
	// This pointer is for chaining
	void* chainPtr = nullptr;

	if (config_.supportBindlessTextures_)
	{
		// Used for gl_DrawID that requires Vulkan 1.1
		/*shaderDrawFeatures_ =
		{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES,
			.shaderDrawParameters = VK_TRUE
		};*/

		// Used for gl_DrawID but requires Vulkan 1.2
		features11_ =
		{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
			.shaderDrawParameters = VK_TRUE
		};

		descriptorIndexingFeatures_ =
		{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
			.pNext = &features11_,
			.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
			.descriptorBindingVariableDescriptorCount = VK_TRUE,
			.runtimeDescriptorArray = VK_TRUE
		};
		chainPtr = &descriptorIndexingFeatures_;
	}

	if (config_.suportBufferDeviceAddress_ || config_.supportRaytracing_)
	{
		deviceAddressEnabledFeatures_ =
		{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
			.pNext = chainPtr,
			.bufferDeviceAddress = VK_TRUE
		};

		chainPtr = &deviceAddressEnabledFeatures_;
	}

	if (config_.supportRaytracing_)
	{
		rtPipelineEnabledFeatures_ =
		{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
			.pNext = chainPtr,
			.rayTracingPipeline = VK_TRUE,
		};

		rtASEnabledFeatures =
		{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
			.pNext = &rtPipelineEnabledFeatures_,
			.accelerationStructure = VK_TRUE,
		};

		chainPtr = &rtASEnabledFeatures;
	}

	features_ = {};
	if (config_.supportMSAA_)
	{
		features_.sampleRateShading = VK_TRUE;
		features_.samplerAnisotropy = VK_TRUE;
	}
	if (config_.supportWideLines_)
	{
		features_.wideLines = VK_TRUE;
	}
	if (config_.supportBindlessTextures_)
	{
		features_.multiDrawIndirect = VK_TRUE;
		features_.drawIndirectFirstInstance = VK_TRUE;
		features_.shaderSampledImageArrayDynamicIndexing = VK_TRUE;
	}

	// NOTE features2_ and features13_ are always created
	features13_ =
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = chainPtr,
		.synchronization2 = true,
	};
	features2_ =
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &features13_,
		.features = features_,
	};
}

void VulkanContext::CreateDevice()
{
	// Add raytracing extensions here
	std::vector<const char*> extensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	if (config_.supportBindlessTextures_)
	{
		extensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
		extensions.push_back(VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME);
		// Used for gl_DrawID but deprecated
		//extensions.push_back(VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME);
	}

	if (config_.suportBufferDeviceAddress_ || config_.supportRaytracing_)
	{
		extensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
	}

	if (config_.supportBindlessTextures_ || config_.supportRaytracing_)
	{
		extensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
	}

	if (config_.supportRaytracing_)
	{
		extensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
		extensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
		extensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
		extensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
		extensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
	}

	const bool sameFamily = graphicsFamily_ == computeFamily_;
	constexpr float priorityOne = 1.f;
	constexpr float priorityZero = 0.f;

	std::vector<VkDeviceQueueCreateInfo> queueInfoArray;
	const VkDeviceQueueCreateInfo graphicsQueueInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = graphicsFamily_,
		.queueCount = 1,
		.pQueuePriorities = sameFamily ? &priorityOne : &priorityZero
	};

	queueInfoArray.push_back(graphicsQueueInfo);

	if (!sameFamily)
	{
		const VkDeviceQueueCreateInfo computeQueueInfo =
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.queueFamilyIndex = computeFamily_,
			.queueCount = 1,
			.pQueuePriorities = &priorityZero
		};
		queueInfoArray.push_back(computeQueueInfo);
	}

	VkDeviceCreateInfo devCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueCreateInfoCount = static_cast<uint32_t>(queueInfoArray.size()),
		.pQueueCreateInfos = queueInfoArray.data(),
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data(),
		.pEnabledFeatures = &features_
	};

	// Chaining
	devCreateInfo.pEnabledFeatures = nullptr;
	// NOTE features2_ is always created
	devCreateInfo.pNext = &features2_; // Features2 is always created

	VK_CHECK(vkCreateDevice(physicalDevice_, &devCreateInfo, nullptr, &device_));
}

VkSampleCountFlagBits VulkanContext::GetMaxUsableSampleCount(VkPhysicalDevice d)
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(d, &physicalDeviceProperties);

	VkSampleCountFlags counts =
		physicalDeviceProperties.limits.framebufferColorSampleCounts &
		physicalDeviceProperties.limits.framebufferDepthSampleCounts;

	VkSampleCountFlagBits max = VK_SAMPLE_COUNT_1_BIT;
	if (counts & VK_SAMPLE_COUNT_64_BIT) { max = VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { max = VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { max = VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { max = VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { max = VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { max = VK_SAMPLE_COUNT_2_BIT; }

	VkSampleCountFlagBits msaaCount = static_cast<VkSampleCountFlagBits>(AppConfig::MSAACount);

	return msaaCount > max ? max : msaaCount;
}

VkResult VulkanContext::CreatePhysicalDevice(VkInstance instance)
{
	uint32_t deviceCount = 0;
	VK_CHECK_RET(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));

	if (!deviceCount)
	{
		throw std::runtime_error("Device count is zero");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	VK_CHECK_RET(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

	//msaaSampleCount_ = VK_SAMPLE_COUNT_1_BIT; // Default
	for (const auto& d : devices)
	{
		if (IsDeviceSuitable(d))
		{
			physicalDevice_ = d;
			msaaSampleCount_ = config_.supportMSAA_ ? GetMaxUsableSampleCount(physicalDevice_) : VK_SAMPLE_COUNT_1_BIT;
			depthFormat_ = FindDepthFormat();
			// Memory properties are used regularly for creating all kinds of buffers
			vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProperties_);
			return VK_SUCCESS;
		}
	}

	return VK_ERROR_INITIALIZATION_FAILED;
}

uint32_t VulkanContext::FindQueueFamilies(VkQueueFlags desiredFlags) const
{
	uint32_t familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &familyCount, nullptr);

	std::vector<VkQueueFamilyProperties> families(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &familyCount, families.data());

	for (uint32_t i = 0; i != families.size(); ++i)
	{
		if (families[i].queueCount > 0 && families[i].queueFlags & desiredFlags)
		{
			return i;
		}
	}

	return 0;
}

VkResult VulkanContext::CreateSwapchain(VkSurfaceKHR surface)
{
	const VkSurfaceFormatKHR surfaceFormat = { swapchainImageFormat_, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	const SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(surface);
	const VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapchainSupport.presentModes_);

	const VkSurfaceCapabilitiesKHR capabilities = swapchainSupport.capabilities_;
	swapchainWidth_ = std::clamp(swapchainWidth_, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	swapchainHeight_ = std::clamp(swapchainHeight_, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	const VkSwapchainCreateInfoKHR createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.flags = 0,
		.surface = surface,
		.minImageCount = GetSwapchainImageCount(swapchainSupport.capabilities_),
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = {.width = swapchainWidth_, .height = swapchainHeight_ },
		.imageArrayLayers = 1,
		.imageUsage =
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &graphicsFamily_,
		.preTransform = swapchainSupport.capabilities_.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE
	};

	return vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_);
}

VkPresentModeKHR VulkanContext::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const VkPresentModeKHR& mode : availablePresentModes)
	{
		if (mode == AppConfig::PresentMode)
		{
			return mode;
		}
	}
	// FIFO will always be supported
	return VK_PRESENT_MODE_FIFO_KHR;
}

uint32_t VulkanContext::GetSwapchainImageCount(const VkSurfaceCapabilitiesKHR& capabilities_)
{
	// Request one additional image to make sure
	// we are not waiting on the GPU to complete any operations
	const uint32_t imageCount = capabilities_.minImageCount + 1;
	const bool imageCountExceeded = capabilities_.maxImageCount > 0 && imageCount > capabilities_.maxImageCount;
	return imageCountExceeded ? capabilities_.maxImageCount : imageCount;
}

VkResult VulkanContext::GetNextSwapchainImage(VkSemaphore nextSwapchainImageSemaphore)
{
	return vkAcquireNextImageKHR(
		device_,
		swapchain_,
		UINT64_MAX,
		// Wait for the swapchain image to become available
		nextSwapchainImageSemaphore,
		VK_NULL_HANDLE,
		&currentSwapchainImageIndex_);
}

size_t VulkanContext::CreateSwapchainImages()
{
	uint32_t imageCount = 0;
	VK_CHECK(vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr));
	swapchainImages_.resize(imageCount);
	swapchainImageViews_.resize(imageCount);
	VK_CHECK(vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, swapchainImages_.data()));

	for (uint32_t i = 0; i < imageCount; i++)
	{
		CreateSwapChainImageView(i, swapchainImageFormat_, VK_IMAGE_ASPECT_COLOR_BIT);
	}
	return static_cast<size_t>(imageCount);
}

void VulkanContext::CreateSwapChainImageView(
	size_t imageIndex,
	VkFormat format,
	VkImageAspectFlags aspectFlags)
{
	const VkImageViewCreateInfo viewInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = swapchainImages_[imageIndex],
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.subresourceRange =
		{
			.aspectMask = aspectFlags,
			.baseMipLevel = 0,
			.levelCount = 1u,
			.baseArrayLayer = 0,
			.layerCount = 1u
		}
	};

	VK_CHECK(vkCreateImageView(device_, &viewInfo, nullptr, &swapchainImageViews_[imageIndex]));
}

void VulkanContext::RecreateSwapchainResources(
	VulkanInstance& instance,
	uint32_t width,
	uint32_t height)
{
	for (size_t i = 0; i < swapchainImages_.size(); ++i)
	{
		vkDestroyImageView(device_, swapchainImageViews_[i], nullptr);
	}
	vkDestroySwapchainKHR(device_, swapchain_, nullptr);

	swapchainWidth_ = width;
	swapchainHeight_ = height;

	VK_CHECK(CreateSwapchain(instance.GetSurface()));
	CreateSwapchainImages();
}

SwapchainSupportDetails VulkanContext::QuerySwapchainSupport(VkSurfaceKHR surface)
{
	SwapchainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface, &details.capabilities_);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface, &formatCount, nullptr);

	if (formatCount)
	{
		details.formats_.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			physicalDevice_,
			surface,
			&formatCount,
			details.formats_.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface, &presentModeCount, nullptr);

	if (presentModeCount)
	{
		details.presentModes_.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface, &presentModeCount, details.presentModes_.data());
	}

	return details;
}

VkResult VulkanContext::CreateSemaphore(VkSemaphore* outSemaphore) const
{
	const VkSemaphoreCreateInfo ci =
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};
	return vkCreateSemaphore(device_, &ci, nullptr, outSemaphore);
}

VkResult VulkanContext::CreateFence(VkFence* fence) const
{
	constexpr VkFenceCreateInfo fenceInfo =
	{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	return vkCreateFence(device_, &fenceInfo, nullptr, fence);
}

VkResult VulkanContext::CreateCommandBuffer(VkCommandPool pool, VkCommandBuffer* commandBuffer) const
{
	const VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};

	return vkAllocateCommandBuffers(device_, &allocInfo, commandBuffer);
}

VkResult VulkanContext::CreateCommandPool(uint32_t family, VkCommandPool* pool) const
{
	const VkCommandPoolCreateInfo cpi =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = family
	};

	return vkCreateCommandPool(device_, &cpi, nullptr, pool);
}

bool VulkanContext::IsDeviceSuitable(VkPhysicalDevice d)
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(d, &deviceProperties);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(d, &deviceFeatures);

	const bool isDiscreteGPU =
		deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
	const bool isIntegratedGPU =
		deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
	const bool isGPU = isDiscreteGPU || isIntegratedGPU;

	return isGPU && deviceFeatures.geometryShader;
}

VkCommandBuffer VulkanContext::BeginOneTimeGraphicsCommand() const
{
	VkCommandBuffer commandBuffer;
	CreateCommandBuffer(graphicsCommandPool_, &commandBuffer);

	constexpr VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr
	};

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	return commandBuffer;
}

void VulkanContext::EndOneTimeGraphicsCommand(VkCommandBuffer commandBuffer) const
{
	vkEndCommandBuffer(commandBuffer);

	const VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = nullptr
	};

	vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue_);
	vkFreeCommandBuffers(device_, graphicsCommandPool_, 1, &commandBuffer);
}

VkCommandBuffer VulkanContext::BeginOneTimeComputeCommand() const
{
	VkCommandBuffer commandBuffer;
	CreateCommandBuffer(computeCommandPool_, &commandBuffer);

	constexpr VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr
	};

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	return commandBuffer;
}

void VulkanContext::EndOneTimeComputeCommand(VkCommandBuffer commandBuffer) const
{
	vkEndCommandBuffer(commandBuffer);

	const VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = nullptr,
		.pWaitDstStageMask = nullptr,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = nullptr
	};

	vkQueueSubmit(computeQueue_, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(computeQueue_);
	vkFreeCommandBuffers(device_, computeCommandPool_, 1, &commandBuffer);
}

void VulkanContext::AllocateFrameInFlightData()
{
	// Frame in flight
	frameIndex_ = 0;
	for (uint32_t i = 0; i < AppConfig::FrameCount; ++i)
	{
		VK_CHECK(CreateSemaphore(&(frameDataArray_[i].nextSwapchainImageSemaphore_)));
		VK_CHECK(CreateSemaphore(&(frameDataArray_[i].graphicsQueueSemaphore_)));
		VK_CHECK(CreateFence(&(frameDataArray_[i].queueSubmitFence_)));
		VK_CHECK(CreateCommandBuffer(graphicsCommandPool_, &(frameDataArray_[i].graphicsCommandBuffer_)));
		frameDataArray_[i].tracyContext_ =
			TracyVkContext(physicalDevice_, device_, graphicsQueue_, frameDataArray_[i].graphicsCommandBuffer_);
	}
}

FrameData& VulkanContext::GetCurrentFrameData()
{
	return frameDataArray_[frameIndex_];
}

void VulkanContext::IncrementFrameIndex()
{
	frameIndex_ = (frameIndex_ + 1u) % AppConfig::FrameCount;
}

uint32_t VulkanContext::GetFrameIndex() const
{
	return frameIndex_;
}

VkFormat VulkanContext::FindDepthFormat() const
{
	return FindSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

VkFormat VulkanContext::FindSupportedFormat(
	const std::vector<VkFormat>& candidates,
	VkImageTiling tiling,
	VkFormatFeatureFlags features) const
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice_, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("Failed to find supported format\n");
}

void VulkanContext::SetVkObjectName(void* objectHandle, VkObjectType objType, const char* name) const
{
	const VkDebugUtilsObjectNameInfoEXT nameInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.pNext = nullptr,
		.objectType = objType,
		.objectHandle = reinterpret_cast<uint64_t>(objectHandle),
		.pObjectName = name
	};
	VK_CHECK(vkSetDebugUtilsObjectNameEXT(device_, &nameInfo));
}

void VulkanContext::InsertDebugLabel(VkCommandBuffer commandBuffer, const char* label, uint32_t colorRGBA) const
{
	const VkDebugUtilsLabelEXT utilsLabel = {
	 .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
	 .pNext = nullptr,
	 .pLabelName = label,
	 .color = {static_cast<float>((colorRGBA >> 0) & 0xff) / 255.0f,
				static_cast<float>((colorRGBA >> 8) & 0xff) / 255.0f,
				static_cast<float>((colorRGBA >> 16) & 0xff) / 255.0f,
				static_cast<float>((colorRGBA >> 24) & 0xff) / 255.0f},
	};
	vkCmdInsertDebugUtilsLabelEXT(commandBuffer, &utilsLabel);
}