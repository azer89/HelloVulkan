#ifndef PIPELINE_IMGUI
#define PIPELINE_IMGUI

#include "PipelineBase.h"
#include "FrameCounter.h"
#include "PushConstants.h"

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
	void ImGuiShowPBRConfig(PushConstPBR* pc, float mipmapCount);
	void ImGuiEnd();
	void ImGuiDrawEmpty();

	void SetCameraUBO(VulkanContext& ctx, CameraUBO& ubo) override {}
	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

private:
	VkDevice device_;
	VkDescriptorPool imguiPool_;
};

#endif