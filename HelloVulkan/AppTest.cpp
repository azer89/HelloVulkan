#include "AppTest.h"
#include "AppSettings.h"
#include "VulkanImage.h"

AppTest::AppTest()
{
}

int AppTest::MainLoop()
{
	VulkanImage depthTexture;
	depthTexture.CreateDepthResources(vulkanDevice, 
		static_cast<uint32_t>(AppSettings::ScreenWidth),
		static_cast<uint32_t>(AppSettings::ScreenHeight));

	while (!GLFWWindowShouldClose())
	{
		PollEvents();
		ProcessTiming();
		ProcessInput();
	}

	depthTexture.Destroy(vulkanDevice.GetDevice());

	Terminate();

	return 0;
}