#ifndef PIPELINE_IMGUI
#define PIPELINE_IMGUI

#include "Camera.h"
#include "PipelineBase.h"
#include "FrameCounter.h"
#include "PushConstants.h"
#include "InputContext.h"

class PipelineImGui final : public PipelineBase
{
public:
	PipelineImGui(
		VulkanContext& ctx, 
		VkInstance vulkanInstance,
		GLFWwindow* glfwWindow);
	~PipelineImGui();

	void ImGuiStart();
	void ImGuiSetWindow(const char* title, int width, int height, float fontSize = 1.0f);
	void ImGuiShowFrameData(FrameCounter* frameCounter);
	void ImGuiShowPBRConfig(PushConstPBR* pc, float mipmapCount);
	void ImGuiEnd();
	void ImGuiDrawEmpty();

	void ImGuizmoStart();
	void ImGuizmoShow(const Camera* camera, glm::mat4& matrix, const int editMode);
	void ImGuizmoShowOption(int* editMode);

	void SetCameraUBO(VulkanContext& ctx, CameraUBO& ubo) override {}
	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;
};

#endif