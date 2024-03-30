#include "VulkanCheck.h"

void CHECK(bool check, const char* fileName, int lineNumber)
{
	if (!check)
	{
		std::cerr << "CHECK() failed filename:" << fileName << ", lineNumber:" << lineNumber << "\n\n";
	}
}