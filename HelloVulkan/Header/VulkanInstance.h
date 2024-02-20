#ifndef VULKAN_INSTANCE
#define VULKAN_INSTANCE

#include "volk.h"

#include "GLFW/glfw3.h"

class VulkanInstance
{
public:
	VulkanInstance() = default;
	~VulkanInstance() = default;

	// Not copyable or movable
	VulkanInstance(const VulkanInstance&) = delete;
	VulkanInstance& operator=(const VulkanInstance&) = delete;
	VulkanInstance(VulkanInstance&&) = delete;
	VulkanInstance& operator=(VulkanInstance&&) = delete;

	void Create();
	void Destroy();

	void SetupDebugCallbacks();
	void CreateWindowSurface(GLFWwindow* glfwWindow_);

	VkInstance GetInstance() const { return instance_; }
	VkSurfaceKHR GetSurface() const { return surface_; }

private:
	VkInstance instance_;
	VkSurfaceKHR surface_;
	VkDebugUtilsMessengerEXT messenger_;
	VkDebugReportCallbackEXT reportCallback_;
};

#endif