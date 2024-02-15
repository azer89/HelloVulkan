#ifndef PIPELINE_IMGUI
#define PIPELINE_IMGUI

#include "PipelineBase.h"

class PipelineImGui final : public PipelineBase
{
public:
	PipelineImGui(
		VulkanContext& vkDev, 
		VkInstance vulkanInstance,
		GLFWwindow* glfwWindow);
	~PipelineImGui();

	void StartImGui();
	void EndImGui();
	void DrawEmptyImGui();

	void FillCommandBuffer(VulkanContext& vkDev, VkCommandBuffer commandBuffer) override;
};

#endif