#ifndef VULKAN_CHECK
#define VULKAN_CHECK

/*
Adapted from
	3D Graphics Rendering Cookbook
	by Sergey Kosarevsky & Viktor Latypov
	https://github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook
*/

#include <iostream>

#include "volk.h"

void CHECK(bool check, const char* fileName, int lineNumber);

#define VK_CHECK(value) CHECK(value == VK_SUCCESS, __FILE__, __LINE__);

#define VK_CHECK_RET(value) if ( value != VK_SUCCESS ) { CHECK(false, __FILE__, __LINE__); return value; }

// This is used in VulkanInstance::SetupDebugCallbacks()
static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT Severity,
	VkDebugUtilsMessageTypeFlagsEXT Type,
	const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
	void* UserData
)
{
	std::cerr << "VulkanDebugCallback: " << CallbackData->pMessage << "\n\n";
	return VK_FALSE;
}

#endif