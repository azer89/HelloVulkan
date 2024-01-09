#include "VulkanDevice.h"
#include "VulkanUtility.h"
#include "AppSettings.h"

void VulkanDevice::CreateCompute
(
	VulkanInstance& instance,
	uint32_t width,
	uint32_t height,
	VkPhysicalDeviceFeatures deviceFeatures
)
{
	framebufferWidth_ = width;
	framebufferHeight_ = height;

	VK_CHECK(CreatePhysicalDevice(instance.GetInstance()));
	graphicsFamily_ = FindQueueFamilies(VK_QUEUE_GRAPHICS_BIT);
	computeFamily_ = FindQueueFamilies(VK_QUEUE_COMPUTE_BIT);
	VK_CHECK(CreateDeviceWithCompute(deviceFeatures, graphicsFamily_, computeFamily_));

	deviceQueueIndices_.push_back(graphicsFamily_);
	if (graphicsFamily_ != computeFamily_)
	{
		deviceQueueIndices_.push_back(computeFamily_);
	}

	vkGetDeviceQueue(device_, graphicsFamily_, 0, &graphicsQueue_);
	if (graphicsQueue_ == nullptr)
	{
		std::cerr << "Cannot get graphics queue\n";
	}

	vkGetDeviceQueue(device_, computeFamily_, 0, &computeQueue_);
	if (computeQueue_ == nullptr)
	{
		std::cerr << "Cannot get compute queue\n";
	}

	VkBool32 presentSupported = 0;
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice_, graphicsFamily_, instance.GetSurface(), &presentSupported);
	if (!presentSupported)
	{
		std::cerr << "Cannot get surface support KHR\n";
	}

	// TODO Create a swapchain class
	VK_CHECK(CreateSwapchain(instance.GetSurface()));
	const size_t imageCount = CreateSwapchainImages();

	const VkCommandPoolCreateInfo cpi =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = graphicsFamily_
	};

	VK_CHECK(vkCreateCommandPool(device_, &cpi, nullptr, &commandPool_));

	// Frame contexts
	frameIndex_ = 0;
	frameContexts_ = std::vector<FrameContext>(AppSettings::FrameOverlapCount);
	for (unsigned int i = 0; i < AppSettings::FrameOverlapCount; ++i)
	{
		VK_CHECK(CreateSemaphore(&(frameContexts_[i].swapchainSemaphore_)));
		VK_CHECK(CreateSemaphore(&(frameContexts_[i].renderSemaphore_)));
		VK_CHECK(CreateFence(&(frameContexts_[i].renderFence_)));
		VK_CHECK(CreateCommandBuffer(commandPool_, &(frameContexts_[i].commandBuffer_)));
	}
	/*swapchainSemaphores_.resize(2);
	renderSemaphores_.resize(2);
	renderFences_.resize(2);
	commandBuffers_.resize(2);
	for (unsigned int i = 0; i < 2; ++i)
	{
		VK_CHECK(CreateSemaphore(&swapchainSemaphores_[i]));
		VK_CHECK(CreateSemaphore(&renderSemaphores_[i]));
		VK_CHECK(CreateFence(&renderFences_[i]));
		VK_CHECK(CreateCommandBuffer(commandPool_, &commandBuffers_[i]));
	}*/
	{
		// Create compute command pool
		const VkCommandPoolCreateInfo cpi1 =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, // Allow command from this pool buffers to be reset
			.queueFamilyIndex = computeFamily_
		};
		VK_CHECK(vkCreateCommandPool(device_, &cpi1, nullptr, &computeCommandPool_));

		CreateCommandBuffer(computeCommandPool_, &computeCommandBuffer_);
	}

	useCompute_ = true;
}

void VulkanDevice::Destroy()
{
	for (unsigned int i = 0; i < AppSettings::FrameOverlapCount; ++i)
	{
		vkDestroySemaphore(device_, frameContexts_[i].swapchainSemaphore_, nullptr);
		vkDestroySemaphore(device_, frameContexts_[i].renderSemaphore_, nullptr);
		vkDestroyFence(device_, frameContexts_[i].renderFence_, nullptr);
	}

	for (size_t i = 0; i < swapchainImages_.size(); i++)
	{
		vkDestroyImageView(device_, swapchainImageViews_[i], nullptr);
	}
	vkDestroySwapchainKHR(device_, swapchain_, nullptr);

	vkDestroyCommandPool(device_, commandPool_, nullptr);

	if (useCompute_)
	{
		vkDestroyCommandPool(device_, computeCommandPool_, nullptr);
	}

	vkDestroyDevice(device_, nullptr);
}

VkSampleCountFlagBits VulkanDevice::GetMaxUsableSampleCount(VkPhysicalDevice d)
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(d, &physicalDeviceProperties);

	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
		physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT)
	{
		return VK_SAMPLE_COUNT_64_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_32_BIT)
	{
		return VK_SAMPLE_COUNT_32_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_16_BIT)
	{
		return VK_SAMPLE_COUNT_16_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_8_BIT)
	{
		return VK_SAMPLE_COUNT_8_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_4_BIT)
	{
		return VK_SAMPLE_COUNT_4_BIT;
	}
	if (counts & VK_SAMPLE_COUNT_2_BIT)
	{
		return VK_SAMPLE_COUNT_2_BIT;
	}

	return VK_SAMPLE_COUNT_1_BIT;
}

VkResult VulkanDevice::CreatePhysicalDevice(VkInstance instance)
{
	uint32_t deviceCount = 0;
	VK_CHECK_RET(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));

	if (!deviceCount)
	{
		std::cerr << "Device count is zero\n";
		return VK_ERROR_INITIALIZATION_FAILED;
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	VK_CHECK_RET(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

	msaaSampleCount_ = VK_SAMPLE_COUNT_1_BIT; // Default
	for (const auto& d : devices)
	{
		if (IsDeviceSuitable(d))
		{
			physicalDevice_ = d;
			msaaSampleCount_ = GetMaxUsableSampleCount(physicalDevice_);
			depthFormat_ = FindDepthFormat();
			return VK_SUCCESS;
		}
	}

	return VK_ERROR_INITIALIZATION_FAILED;
}

uint32_t VulkanDevice::FindQueueFamilies(VkQueueFlags desiredFlags)
{
	uint32_t familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &familyCount, nullptr);

	std::vector<VkQueueFamilyProperties> families(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &familyCount, families.data());

	for (uint32_t i = 0; i != families.size(); i++)
	{
		if (families[i].queueCount > 0 && families[i].queueFlags & desiredFlags)
		{
			return i;
		}
	}

	return 0;
}

VkResult VulkanDevice::CreateDeviceWithCompute(
	VkPhysicalDeviceFeatures deviceFeatures, 
	uint32_t graphicsFamily, 
	uint32_t computeFamily)
{
	const std::vector<const char*> extensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	if (graphicsFamily == computeFamily)
	{
		return CreateDevice(deviceFeatures, graphicsFamily);
	}

	const float queuePriorities[2] = { 0.f, 0.f };
	const VkDeviceQueueCreateInfo qciGfx =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = graphicsFamily,
		.queueCount = 1,
		.pQueuePriorities = &queuePriorities[0]
	};

	const VkDeviceQueueCreateInfo qciComp =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = computeFamily,
		.queueCount = 1,
		.pQueuePriorities = &queuePriorities[1]
	};

	const VkDeviceQueueCreateInfo qci[2] = { qciGfx, qciComp };

	const VkDeviceCreateInfo ci =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueCreateInfoCount = 2,
		.pQueueCreateInfos = qci,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data(),
		.pEnabledFeatures = &deviceFeatures
	};

	return vkCreateDevice(physicalDevice_, &ci, nullptr, &device_);
}

VkResult VulkanDevice::CreateDevice(VkPhysicalDeviceFeatures deviceFeatures, uint32_t graphicsFamily)
{
	const std::vector<const char*> extensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME
	};

	const float queuePriority = 1.0f;
	const VkDeviceQueueCreateInfo queueCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = graphicsFamily,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority
	};

	const VkDeviceCreateInfo deviceCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queueCreateInfo,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data(),
		.pEnabledFeatures = &deviceFeatures
	};

	return vkCreateDevice(physicalDevice_, &deviceCreateInfo, nullptr, &device_);
}

VkResult VulkanDevice::CreateSwapchain(VkSurfaceKHR surface)
{
	swapchainImageFormat_ = VK_FORMAT_B8G8R8A8_UNORM;
	VkSurfaceFormatKHR surfaceFormat = { swapchainImageFormat_, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	auto swapchainSupport = QuerySwapchainSupport(surface);
	auto presentMode = ChooseSwapPresentMode(swapchainSupport.presentModes);

	const VkSwapchainCreateInfoKHR createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.flags = 0,
		.surface = surface,
		.minImageCount = GetSwapchainImageCount(swapchainSupport.capabilities),
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = {.width = framebufferWidth_, .height = framebufferHeight_ },
		.imageArrayLayers = 1,
		.imageUsage = 
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | 
			VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &graphicsFamily_,
		.preTransform = swapchainSupport.capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE
	};

	return vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_);
}

VkPresentModeKHR VulkanDevice::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const VkPresentModeKHR& mode : availablePresentModes)
	{
		if (mode == AppSettings::PresentMode)
		{
			return mode;
		}
	}
	// FIFO will always be supported
	return VK_PRESENT_MODE_FIFO_KHR;
}

uint32_t VulkanDevice::GetSwapchainImageCount(const VkSurfaceCapabilitiesKHR& capabilities)
{
	// Request one additional image to make sure
	// we are not waiting on the GPU to complete any operations
	const uint32_t imageCount = capabilities.minImageCount + 1;
	const bool imageCountExceeded = capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount;
	return imageCountExceeded ? capabilities.maxImageCount : imageCount;
}


size_t VulkanDevice::CreateSwapchainImages()
{
	uint32_t imageCount = 0;
	VK_CHECK(vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr));
	swapchainImages_.resize(imageCount);
	swapchainImageViews_.resize(imageCount);
	VK_CHECK(vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, swapchainImages_.data()));

	for (unsigned i = 0; i < imageCount; i++)
	{
		if (!CreateSwapChainImageView(i, swapchainImageFormat_, VK_IMAGE_ASPECT_COLOR_BIT))
		{
			throw std::runtime_error("Cannot create swapchain image view\n");
		}
	}
	return static_cast<size_t>(imageCount);
}

bool VulkanDevice::CreateSwapChainImageView(
	unsigned imageIndex,
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

	return (vkCreateImageView(device_, &viewInfo, nullptr, &swapchainImageViews_[imageIndex]) == VK_SUCCESS);
}

void VulkanDevice::RecreateSwapchainResources(
	VulkanInstance& instance,
	uint32_t width,
	uint32_t height)
{
	for (size_t i = 0; i < swapchainImages_.size(); ++i)
	{
		vkDestroyImageView(device_, swapchainImageViews_[i], nullptr);
	}
	vkDestroySwapchainKHR(device_, swapchain_, nullptr);

	framebufferWidth_ = width;
	framebufferHeight_ = height;

	VK_CHECK(CreateSwapchain(instance.GetSurface()));
	CreateSwapchainImages();
}

SwapchainSupportDetails VulkanDevice::QuerySwapchainSupport(VkSurfaceKHR surface)
{
	SwapchainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface, &formatCount, nullptr);

	if (formatCount)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			physicalDevice_, 
			surface, 
			&formatCount, 
			details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface, &presentModeCount, nullptr);

	if (presentModeCount)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkResult VulkanDevice::CreateSemaphore(VkSemaphore* outSemaphore)
{
	const VkSemaphoreCreateInfo ci =
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	return vkCreateSemaphore(device_, &ci, nullptr, outSemaphore);
}

VkResult VulkanDevice::CreateFence(VkFence* fence)
{
	VkFenceCreateInfo fenceInfo =
	{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	return vkCreateFence(device_, &fenceInfo, nullptr, fence);
}

VkResult VulkanDevice::CreateCommandBuffer(VkCommandPool pool, VkCommandBuffer* commandBuffer)
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

FrameContext& VulkanDevice::GetFrameContext()
{
	return frameContexts_[frameIndex_];
}

void VulkanDevice::IncrementFrameIndex()
{
	frameIndex_ = (frameIndex_ + 1) % 2;
}

bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice d)
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

VkFormat VulkanDevice::FindDepthFormat()
{
	return FindSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

VkFormat VulkanDevice::FindSupportedFormat(
	const std::vector<VkFormat>& candidates,
	VkImageTiling tiling,
	VkFormatFeatureFlags features)
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

	std::cerr << "Failed to find supported format\n";
	exit(0);
}

VkCommandBuffer VulkanDevice::BeginSingleTimeCommands()
{
	VkCommandBuffer commandBuffer;
	CreateCommandBuffer(commandPool_, &commandBuffer);

	const VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = nullptr
	};

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VulkanDevice::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
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

	vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer);
}

void VulkanDevice::SetVkObjectName(void* objectHandle, VkObjectType objType, const char* name)
{
	VkDebugUtilsObjectNameInfoEXT nameInfo = {
		.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.pNext = nullptr,
		.objectType = objType,
		.objectHandle = reinterpret_cast<uint64_t>(objectHandle),
		.pObjectName = name
	};
	VK_CHECK(vkSetDebugUtilsObjectNameEXT(device_, &nameInfo));
}