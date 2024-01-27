#ifndef RENDERER_IMGUI
#define RENDERER_IMGUI

#include "PipelineBase.h"

class RendererImGui final : public PipelineBase
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