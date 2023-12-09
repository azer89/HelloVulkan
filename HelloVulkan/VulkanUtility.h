#ifndef VULKAN_UTILITY
#define VULKAN_UTILITY

#include <iostream>

#define VK_NO_PROTOTYPES
#include "volk.h"

void CHECK(bool check, const char* fileName, int lineNumber)
{
	if (!check)
	{
		std::cerr << "CHECK() failed at filename:" << fileName << ", lineNumber:" << lineNumber;
	}
}

#define VK_CHECK(value) CHECK(value == VK_SUCCESS, __FILE__, __LINE__);
#define VK_CHECK_RET(value) if ( value != VK_SUCCESS ) { CHECK(false, __FILE__, __LINE__); return value; }
#define BL_CHECK(value) CHECK(value, __FILE__, __LINE__);

#endif