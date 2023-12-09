#include "VulkanUtility.h"

void CHECK(bool check, const char* fileName, int lineNumber)
{
	if (!check)
	{
		std::cerr << "CHECK() failed at filename:" << fileName << ", lineNumber:" << lineNumber;
	}
}