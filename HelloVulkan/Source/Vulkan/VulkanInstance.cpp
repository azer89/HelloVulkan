#include "VulkanInstance.h"
#include "VulkanCheck.h"
#include "Configs.h"

#include <vector>

void VulkanInstance::Create()
{
	// https://vulkan.lunarg.com/doc/view/1.1.108.0/windows/validation_layers.html
	const std::vector<const char*> vLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};

	uint32_t glfwExtensionCount;
	auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	const VkApplicationInfo appInfo =
	{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = AppConfig::ScreenTitle.c_str(),
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "Hello Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_3
	};

	const VkInstanceCreateInfo createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = static_cast<uint32_t>(vLayers.size()),
		.ppEnabledLayerNames = vLayers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data()
	};

	VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance_));

	// We use Volk to obtain a Vulkan instance
	volkLoadInstance(instance_);
}

void VulkanInstance::Destroy()
{
	vkDestroySurfaceKHR(instance_, surface_, nullptr);

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
}

void VulkanInstance::CreateWindowSurface(GLFWwindow* glfwWindow_)
{
	VK_CHECK(glfwCreateWindowSurface(instance_, glfwWindow_, nullptr, &surface_));
}