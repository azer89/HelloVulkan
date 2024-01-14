#ifndef RENDERER_IMGUI
#define RENDERER_IMGUI

#include "RendererBase.h"

class RendererImGui final : public RendererBase
{
public:
	RendererImGui(
		VulkanDevice& vkDev, 
		VkInstance vulkanInstance,
		GLFWwindow* glfwWindow);
	~RendererImGui();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;
};

#endif