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

	VkInstance GetInstance() { return instance_; }
	VkSurfaceKHR GetSurface() { return surface_; }

private:
	VkInstance instance_;
	VkSurfaceKHR surface_;
	VkDebugUtilsMessengerEXT messenger_;
	VkDebugReportCallbackEXT reportCallback_;
};

#endif