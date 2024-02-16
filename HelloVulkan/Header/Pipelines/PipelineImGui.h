#ifndef PIPELINE_IMGUI
#define PIPELINE_IMGUI

#include "PipelineBase.h"

class PipelineImGui final : public PipelineBase
{
public:
	PipelineImGui(
		VulkanContext& ctx, 
		VkInstance vulkanInstance,
		GLFWwindow* glfwWindow);
	~PipelineImGui();

	void StartImGui();
	void EndImGui();
	void DrawEmptyImGui();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;
};

#endif