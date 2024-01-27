#ifndef RENDERER_IMGUI
#define RENDERER_IMGUI

#include "PipelineBase.h"

class PipelineImGui final : public PipelineBase
{
public:
	PipelineImGui(
		VulkanDevice& vkDev, 
		VkInstance vulkanInstance,
		GLFWwindow* glfwWindow);
	~PipelineImGui();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;
};

#endif