#include "VulkanInstance.h"
#include "VulkanUtility.h"

#include <vector>

VulkanInstance::VulkanInstance()
{
}

VulkanInstance::~VulkanInstance()
{
}

void VulkanInstance::Create()
{
	// https://vulkan.lunarg.com/doc/view/1.1.108.0/windows/validation_layers.html
	const std::vector<const char*> ValidationLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> exts =
	{
		"VK_KHR_surface",
		"VK_KHR_win32_surface",
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		/* for indexed textures */
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
	};

	const VkApplicationInfo appinfo =
	{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = "Vulkan",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_1
	};

	const VkInstanceCreateInfo createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &appinfo,
		.enabledLayerCount = static_cast<uint32_t>(ValidationLayers.size()),
		.ppEnabledLayerNames = ValidationLayers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(exts.size()),
		.ppEnabledExtensionNames = exts.data()
	};

	VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));

	volkLoadInstance(instance);
}

void VulkanInstance::Destroy()
{
	vkDestroySurfaceKHR(instance, surface, nullptr);

	vkDestroyDebugReportCallbackEXT(instance, reportCallback, nullptr);
	vkDestroyDebugUtilsMessengerEXT(instance, messenger, nullptr);

	vkDestroyInstance(instance, nullptr);
}