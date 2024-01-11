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
	constexpr unsigned int OutputDiffuseSampleCount = 1024;
	constexpr uint32_t InputCubeSideLength = 1024;
	constexpr uint32_t OutputDiffuseSideLength = 32;
	constexpr uint32_t OutputSpecularSideLength = 128;
	constexpr VkFormat CubeFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
	constexpr uint32_t LayerCount = 6;

	// BRDF LUT
	// TODO Use push constants to send the image dimension
	constexpr int LUTWidth = 256;
	constexpr int LUTHeight = 256;
	constexpr uint32_t LUTBufferSize = 2 * sizeof(float) * LUTWidth * LUTHeight;
}

#endif