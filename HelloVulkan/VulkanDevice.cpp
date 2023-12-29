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
	framebufferWidth = width;
	framebufferHeight = height;

	VK_CHECK(CreatePhysicalDevice(instance.GetInstance()));
	graphicsFamily = FindQueueFamilies(VK_QUEUE_GRAPHICS_BIT);
	computeFamily = FindQueueFamilies(VK_QUEUE_COMPUTE_BIT);
	VK_CHECK(CreateDeviceWithCompute(deviceFeatures, graphicsFamily, computeFamily));

	deviceQueueIndices.push_back(graphicsFamily);
	if (graphicsFamily != computeFamily)
		deviceQueueIndices.push_back(computeFamily);

	vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
	if (graphicsQueue == nullptr)
		std::cerr << "Cannot get graphics queue\n";

	vkGetDeviceQueue(device, computeFamily, 0, &computeQueue);
	if (computeQueue == nullptr)
		std::cerr << "Cannot get compute queue\n";

	VkBool32 presentSupported = 0;
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, graphicsFamily, instance.GetSurface(), &presentSupported);
	if (!presentSupported)
		std::cerr << "Cannot get surface support KHR\n";

	// TODO Create a swapchain class
	VK_CHECK(CreateSwapchain(instance.GetSurface()));
	const size_t imageCount = CreateSwapchainImages();
	commandBuffers.resize(imageCount);

	VK_CHECK(CreateSemaphore(&semaphore));
	VK_CHECK(CreateSemaphore(&renderSemaphore));

	const VkCommandPoolCreateInfo cpi =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = 0,
		.queueFamilyIndex = graphicsFamily
	};

	VK_CHECK(vkCreateCommandPool(device, &cpi, nullptr, &commandPool));

	const VkCommandBufferAllocateInfo ai =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = static_cast<uint32_t>(swapchainImages.size()),
	};

	VK_CHECK(vkAllocateCommandBuffers(device, &ai, &commandBuffers[0]));

	{
		// Create compute command pool
		const VkCommandPoolCreateInfo cpi1 =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, // Allow command from this pool buffers to be reset
			.queueFamilyIndex = computeFamily
		};
		VK_CHECK(vkCreateCommandPool(device, &cpi1, nullptr, &computeCommandPool));

		const VkCommandBufferAllocateInfo ai1 =
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.pNext = nullptr,
			.commandPool = computeCommandPool,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};

		VK_CHECK(vkAllocateCommandBuffers(device, &ai1, &computeCommandBuffer));
	}

	useCompute = true;
}

void VulkanDevice::Destroy()
{
	for (size_t i = 0; i < swapchainImages.size(); i++)
		vkDestroyImageView(device, swapchainImageViews[i], nullptr);

	vkDestroySwapchainKHR(device, swapchain, nullptr);

	vkDestroyCommandPool(device, commandPool, nullptr);

	vkDestroySemaphore(device, semaphore, nullptr);
	vkDestroySemaphore(device, renderSemaphore, nullptr);

	if (useCompute)
	{
		vkDestroyCommandPool(device, computeCommandPool, nullptr);
	}

	vkDestroyDevice(device, nullptr);
}

VkResult VulkanDevice::CreatePhysicalDevice(VkInstance instance)
{
	uint32_t deviceCount = 0;
	VK_CHECK_RET(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));

	if (!deviceCount) return VK_ERROR_INITIALIZATION_FAILED;

	std::vector<VkPhysicalDevice> devices(deviceCount);
	VK_CHECK_RET(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

	for (const auto& d : devices)
	{
		if (IsDeviceSuitable(d))
		{
			physicalDevice = d;
			msaaSamples = GetMaxUsableSampleCount(d);
			return VK_SUCCESS;
		}
	}

	return VK_ERROR_INITIALIZATION_FAILED;
}

uint32_t VulkanDevice::FindQueueFamilies(VkQueueFlags desiredFlags)
{
	uint32_t familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, nullptr);

	std::vector<VkQueueFamilyProperties> families(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, families.data());

	for (uint32_t i = 0; i != families.size(); i++)
		if (families[i].queueCount > 0 && families[i].queueFlags & desiredFlags)
			return i;

	return 0;
}

VkResult VulkanDevice::CreateDeviceWithCompute(VkPhysicalDeviceFeatures deviceFeatures, uint32_t graphicsFamily, uint32_t computeFamily)
{
	const std::vector<const char*> extensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	if (graphicsFamily == computeFamily)
		return CreateDevice(deviceFeatures, graphicsFamily);

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

	return vkCreateDevice(physicalDevice, &ci, nullptr, &device);
}

VkResult VulkanDevice::CreateDevice(VkPhysicalDeviceFeatures deviceFeatures, uint32_t graphicsFamily)
{
	const std::vector<const char*> extensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME
	};

	const float queuePriority = 1.0f;

	const VkDeviceQueueCreateInfo qci =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueFamilyIndex = graphicsFamily,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority
	};

	const VkDeviceCreateInfo ci =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &qci,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = nullptr,
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data(),
		.pEnabledFeatures = &deviceFeatures
	};

	return vkCreateDevice(physicalDevice, &ci, nullptr, &device);
}

VkResult VulkanDevice::CreateSwapchain(VkSurfaceKHR surface, bool supportScreenshots)
{
	auto swapchainSupport = QuerySwapchainSupport(surface);
	VkSurfaceFormatKHR surfaceFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	auto presentMode = ChooseSwapPresentMode(swapchainSupport.presentModes);

	const VkSwapchainCreateInfoKHR ci =
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.flags = 0,
		.surface = surface,
		.minImageCount = ChooseSwapImageCount(swapchainSupport.capabilities),
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = {.width = framebufferWidth, .height = framebufferHeight },
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | (supportScreenshots ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0u),
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &graphicsFamily,
		.preTransform = swapchainSupport.capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE
	};

	return vkCreateSwapchainKHR(device, &ci, nullptr, &swapchain);
}

VkPresentModeKHR VulkanDevice::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto mode : availablePresentModes)
	{
		if (mode == AppSettings::PresentMode)
		{
			return mode;
		}
	}
	// FIFO will always be supported
	return VK_PRESENT_MODE_FIFO_KHR;
}

uint32_t VulkanDevice::ChooseSwapImageCount(const VkSurfaceCapabilitiesKHR& capabilities)
{
	const uint32_t imageCount = capabilities.minImageCount + 1;

	const bool imageCountExceeded = capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount;

	return imageCountExceeded ? capabilities.maxImageCount : imageCount;
}


size_t VulkanDevice::CreateSwapchainImages()
{
	uint32_t imageCount = 0;
	VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));

	swapchainImages.resize(imageCount);
	swapchainImageViews.resize(imageCount);

	VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()));

	for (unsigned i = 0; i < imageCount; i++)
		if (!CreateImageView(i, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT))
			exit(0);

	return static_cast<size_t>(imageCount);
}

bool VulkanDevice::CreateImageView(unsigned imageIndex, VkFormat format, VkImageAspectFlags aspectFlags, VkImageViewType viewType, uint32_t layerCount, uint32_t mipLevels)
{
	const VkImageViewCreateInfo viewInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = swapchainImages[imageIndex],
		.viewType = viewType,
		.format = format,
		.subresourceRange =
		{
			.aspectMask = aspectFlags,
			.baseMipLevel = 0,
			.levelCount = mipLevels,
			.baseArrayLayer = 0,
			.layerCount = layerCount
		}
	};

	return (vkCreateImageView(device, &viewInfo, nullptr, &swapchainImageViews[imageIndex]) == VK_SUCCESS);
}

SwapchainSupportDetails VulkanDevice::QuerySwapchainSupport(VkSurfaceKHR surface)
{
	SwapchainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

	if (formatCount)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

	if (presentModeCount)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkResult VulkanDevice::CreateSemaphore(VkSemaphore* outSemaphore)
{
	const VkSemaphoreCreateInfo ci =
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	return vkCreateSemaphore(device, &ci, nullptr, outSemaphore);
}

bool VulkanDevice::IsDeviceSuitable(VkPhysicalDevice d)
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(d, &deviceProperties);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(d, &deviceFeatures);

	const bool isDiscreteGPU = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
	const bool isIntegratedGPU = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
	const bool isGPU = isDiscreteGPU || isIntegratedGPU;

	return isGPU && deviceFeatures.geometryShader;
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
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	printf("failed to find supported format!\n");
	exit(0);
}

VkCommandBuffer VulkanDevice::BeginSingleTimeCommands()
{
	VkCommandBuffer commandBuffer;

	const VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};

	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

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

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}