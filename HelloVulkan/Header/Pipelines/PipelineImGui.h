#ifndef PIPELINE_IMGUI
#define PIPELINE_IMGUI

#include "Scene.h"
#include "Camera.h"
#include "PipelineBase.h"
#include "FrameCounter.h"
#include "PushConstants.h"
#include "InputContext.h"

class PipelineImGui final : public PipelineBase
{
public:
	// ImGuizmo only works if scene and camera are provided
	PipelineImGui(
		VulkanContext& ctx, 
		VkInstance vulkanInstance,
		GLFWwindow* glfwWindow,
		Scene* scene = nullptr,
		const Camera* camera = nullptr);
	~PipelineImGui();

	void ImGuiStart();
	void ImGuiSetWindow(const char* title, int width, int height, float fontSize = 1.0f);
	void ImGuiShowFrameData(FrameCounter* frameCounter);
	void ImGuiShowPBRConfig(PushConstPBR* pc, float mipmapCount);
	void ImGuiEnd();
	void ImGuiDrawEmpty();

	void ImGuizmoStart();
	void ImGuizmoShow(glm::mat4& modelMatrix, const int editMode);
	void ImGuizmoShowOption(int* editMode);
	void ImGuizmoManipulateScene(VulkanContext& ctx, InputContext* inputContext);

	void SetCameraUBO(VulkanContext& ctx, CameraUBO& ubo) override {}
	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

private:
	Scene* scene_ = nullptr;
	const Camera* camera_ = nullptr;
};

#endif