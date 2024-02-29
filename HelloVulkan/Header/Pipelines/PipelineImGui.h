#ifndef PIPELINE_IMGUI
#define PIPELINE_IMGUI

#include "PipelineBase.h"

class FrameCounter;
struct PushConstantPBR;

class PipelineImGui final : public PipelineBase
{
public:
	PipelineImGui(
		VulkanContext& ctx, 
		VkInstance vulkanInstance,
		GLFWwindow* glfwWindow);
	~PipelineImGui();

	void ImGuiStart();
	void ImGuiSetWindow(const char* title, int width, int height, float fontSize = 1.25f);
	void ImGuiShowFrameData(FrameCounter* frameCounter);
	void ImGuiShowPBRConfig(PushConstantPBR* pc, float mipmapCount);
	void ImGuiEnd();
	void ImGuiDrawEmpty();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;
};

#endif