#include "AppPBR.h"

// Entry point
int main()
{
	VkResult res = volkInitialize();
	if (res != VK_SUCCESS)
	{
		std::cerr << "Volk Cannot be initialized\n";
	}

	AppPBR app;
	auto returnValue = app.MainLoop();
	return returnValue;
}