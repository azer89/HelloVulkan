#ifndef VULKAN_UTILITY
#define VULKAN_UTILITY

/*
Adapted from
	3D Graphics Rendering Cookbook
	by Sergey Kosarevsky & Viktor Latypov
	https://github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook
*/

#include <iostream>
#include <random>

#include "volk.h"

void CHECK(bool check, const char* fileName, int lineNumber);

#ifndef VK_CHECK
#define VK_CHECK(value) CHECK(value == VK_SUCCESS, __FILE__, __LINE__);
#endif

#ifndef VK_CHECK_RET
#define VK_CHECK_RET(value) if ( value != VK_SUCCESS ) { CHECK(false, __FILE__, __LINE__); return value; }
#endif

#ifndef BL_CHECK
#define BL_CHECK(value) CHECK(value, __FILE__, __LINE__);
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT Severity,
	VkDebugUtilsMessageTypeFlagsEXT Type,
	const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
	void* UserData
)
{
	printf("Validation layer: %s\n\n", CallbackData->pMessage);
	return VK_FALSE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugReportCallback
(
	VkDebugReportFlagsEXT      flags,
	VkDebugReportObjectTypeEXT objectType,
	uint64_t                   object,
	size_t                     location,
	int32_t                    messageCode,
	const char* pLayerPrefix,
	const char* pMessage,
	void* UserData
)
{
	// https://github.com/zeux/niagara/blob/master/src/device.cpp   [ignoring performance warnings]
	// This silences warnings like "For optimal performance image layout should be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL instead of GENERAL."
	if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
	{
		return VK_FALSE;
	}

	std::cerr << "Debug callback (" << pLayerPrefix << "): " << pMessage << "\n\n";
	return VK_FALSE;
}

namespace Utility
{
	template <typename T>
	inline T RandomNumber()
	{
		static std::uniform_real_distribution<T> distribution(0.0, 1.0);
		static std::random_device rd;
		static std::mt19937 generator(1000);
		return distribution(generator);
	}

	// Returns a random real in [min,max)
	template <typename T>
	inline T RandomNumber(T min, T max)
	{
		return min + (max - min) * RandomNumber<T>();
	}
}

#endif