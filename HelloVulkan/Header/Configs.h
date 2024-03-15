#ifndef APP_SETTINGS
#define APP_SETTINGS

#include <string>

#include "volk.h"

namespace AppConfig
{
	constexpr uint32_t InitialScreenWidth = 1600;
	constexpr uint32_t InitialScreenHeight = 1200;

	// Number of overlapping frames (Frame in flight)
	constexpr uint32_t FrameOverlapCount = 2;

	// VK_PRESENT_MODE_FIFO_KHR --> Lock to screen FPS
	// VK_PRESENT_MODE_MAILBOX_KHR --> Triple buffering
	constexpr VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;
	
	const std::string ScreenTitle = "Hello Vulkan";

	// Need to change these absolute paths
	const std::string ShaderFolder = "C:/Users/azer/workspace/HelloVulkan/Shaders/";
	const std::string ModelFolder = "C:/Users/azer/workspace/HelloVulkan/Assets/Models/";
	const std::string TextureFolder = "C:/Users/azer/workspace/HelloVulkan/Assets/Textures/";
};

namespace CameraConfig
{
	constexpr float Yaw = -90.0f;
	constexpr float Pitch = 0.0f;
	constexpr float Speed = 2.5f;
	constexpr float Sensitivity = 0.1f;
	constexpr float Zoom = 45.0f;

	constexpr float Near = 0.1f;
	constexpr float Far = 100.0f;
}

namespace ClusterForwardConfig
{
	// Parameters similar to Doom 2016
	constexpr uint32_t sliceCountX = 16;
	constexpr uint32_t sliceCountY = 9;
	constexpr uint32_t sliceCountZ = 24;
	constexpr uint32_t numClusters = sliceCountX * sliceCountY * sliceCountZ;

	// Note that this also has to be set inside the compute shader
	constexpr uint32_t maxLightPerCluster = 150;
}

namespace IBLConfig
{
	constexpr uint32_t OutputDiffuseSampleCount = 1024;
	constexpr uint32_t InputCubeSideLength = 1024;
	constexpr uint32_t OutputDiffuseSideLength = 32;
	constexpr uint32_t OutputSpecularSideLength = 128;
	constexpr uint32_t LayerCount = 6;
	constexpr VkFormat CubeFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

	// BRDF LUT
	constexpr uint32_t LUTSampleCount = 1024;
	constexpr uint32_t LUTWidth = 256;
	constexpr uint32_t LUTHeight = 256;
	constexpr uint32_t LUTBufferSize = 2 * sizeof(float) * LUTWidth * LUTHeight;
}

namespace ShadowConfig
{
	constexpr uint32_t DepthSize = 4096;
}

#endif