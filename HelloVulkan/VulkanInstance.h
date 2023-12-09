#ifndef VULKAN_INSTANCE
#define VULKAN_INSTANCE

#define VK_NO_PROTOTYPES
#include "volk.h"

class VulkanInstance
{
public:
	VulkanInstance();
	~VulkanInstance();

	void Create();
	void Destroy();

	VkInstance GetInstance() { return instance; }

private:
	VkInstance instance;
	VkSurfaceKHR surface;
	VkDebugUtilsMessengerEXT messenger;
	VkDebugReportCallbackEXT reportCallback;
};

#endif