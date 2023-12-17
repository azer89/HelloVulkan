#ifndef VULKAN_INSTANCE
#define VULKAN_INSTANCE

#include "volk.h"

#include "GLFW/glfw3.h"

class VulkanInstance
{
public:
	VulkanInstance();
	~VulkanInstance();

	void Create();
	void Destroy();

	void SetupDebugCallbacks();
	void CreateWindowSurface(GLFWwindow* glfwWindow);

	VkInstance GetInstance() { return instance; }
	VkSurfaceKHR GetSurface() { return surface; }

private:
	VkInstance instance;
	VkSurfaceKHR surface;
	VkDebugUtilsMessengerEXT messenger;
	VkDebugReportCallbackEXT reportCallback;
};

#endif