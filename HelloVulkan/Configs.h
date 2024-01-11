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

	// Number of overlapping frames when rendering
	const unsigned int FrameOverlapCount = 2;
	
	const std::string ScreenTitle = "Hello Vulkan";
	const std::string ShaderFolder = "C://Users//azer//workspace//HelloVulkan//Shaders//";
	const std::string ModelFolder = "C://Users//azer//workspace//HelloVulkan//Models//";
	const std::string TextureFolder = "C://Users//azer//workspace//HelloVulkan//Textures//";
};

namespace IBLConfig
{
	constexpr unsigned int outputDiffuseSampleCount = 1024;
	constexpr uint32_t inputCubeSideLength = 1024;
	constexpr uint32_t outputDiffuseSideLength = 32;
	constexpr uint32_t outputSpecularSideLength = 128;
	constexpr VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
	constexpr uint32_t layerCount = 6;

	// BRDF LUT
	// TODO Use push constants to send the image dimension
	constexpr int lutWidth = 256;
	constexpr int lutHeight = 256;
	constexpr uint32_t lutBufferSize = 2 * sizeof(float) * lutWidth * lutHeight;
}

#endif