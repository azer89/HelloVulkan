#ifndef APP_SETTINGS
#define APP_SETTINGS

#include <string>

#include "volk.h"

namespace AppSettings
{
	const int InitialScreenWidth = 1600;
	const int InitialScreenHeight = 1200;

	// VK_PRESENT_MODE_FIFO_KHR --> Lock to screen FPS
	// VK_PRESENT_MODE_MAILBOX_KHR --> Triple buffering
	const VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;

	// Number of frames to overlap when rendering
	//constexpr unsigned int FrameOverlap = 2;
	
	const std::string ScreenTitle = "Hello Vulkan";
	const std::string ShaderFolder = "C://Users//azer//workspace//HelloVulkan//Shaders//";
	const std::string ModelFolder = "C://Users//azer//workspace//HelloVulkan//Models//";
	const std::string TextureFolder = "C://Users//azer//workspace//HelloVulkan//Textures//";
};

#endif