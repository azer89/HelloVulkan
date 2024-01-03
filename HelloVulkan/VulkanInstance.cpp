#include "VulkanInstance.h"
#include "VulkanUtility.h"
#include "AppSettings.h"

#include <vector>

void VulkanInstance::Create()
{
	// https://vulkan.lunarg.com/doc/view/1.1.108.0/windows/validation_layers.html
	const std::vector<const char*> vLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};

	// Thic projects only works on Windows
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
		.pApplicationName = AppSettings::ScreenTitle.c_str(),
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
		.enabledLayerCount = static_cast<uint32_t>(vLayers.size()),
		.ppEnabledLayerNames = vLayers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(exts.size()),
		.ppEnabledExtensionNames = exts.data()
	};

	VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance_));

	// We use Volk to obtain a Vulkan instance
	volkLoadInstance(instance_);
}

void VulkanInstance::Destroy()
{
	vkDestroySurfaceKHR(instance_, surface_, nullptr);

	vkDestroyDebugReportCallbackEXT(instance_, reportCallback_, nullptr);
	vkDestroyDebugUtilsMessengerEXT(instance_, messenger_, nullptr);

	vkDestroyInstance(instance_, nullptr);
}

void VulkanInstance::SetupDebugCallbacks()
{
	{
		const VkDebugUtilsMessengerCreateInfoEXT ci = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity =
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType =
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = &VulkanDebugCallback,
			.pUserData = nullptr
		};

		VK_CHECK(vkCreateDebugUtilsMessengerEXT(instance_, &ci, nullptr, &messenger_));
	}
	{
		const VkDebugReportCallbackCreateInfoEXT ci = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
			.pNext = nullptr,
			.flags =
				VK_DEBUG_REPORT_WARNING_BIT_EXT |
				VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
				VK_DEBUG_REPORT_ERROR_BIT_EXT |
				VK_DEBUG_REPORT_DEBUG_BIT_EXT,
			.pfnCallback = &VulkanDebugReportCallback,
			.pUserData = nullptr
		};

		VK_CHECK(vkCreateDebugReportCallbackEXT(instance_, &ci, nullptr, &reportCallback_));
	}
}

void VulkanInstance::CreateWindowSurface(GLFWwindow* glfwWindow_)
{
	VK_CHECK(glfwCreateWindowSurface(instance_, glfwWindow_, nullptr, &surface_));
}