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

namespace Utility
{
	inline float RandomNumber()
	{
		static std::uniform_real_distribution<float> distribution(0.0, 1.0);
		static std::random_device rd;
		static std::mt19937 generator(rd());
		return distribution(generator);
	}

	inline float RandomNumber(float min, float max)
	{
		return min + (max - min) * RandomNumber();
	}

	inline uint32_t AlignedSize(uint32_t value, uint32_t alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}

	inline size_t AlignedSize(size_t value, size_t alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}

	inline int MipMapCount(int w, int h)
	{
		int levels = 1;
		while ((w | h) >> levels)
		{
			levels += 1;
		}
		return levels;
	}

	inline int MipMapCount(int size)
	{
		int levels = 1;
		while (size >> levels)
		{
			levels += 1;
		}
		return levels;
	}
}

#endif