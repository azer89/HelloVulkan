#include "PipelineImGui.h"
#include "VulkanCheck.h"
#include "FrameCounter.h"
#include "PushConstants.h"
#include "Configs.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "ImGuizmo.h"
#include <glm/gtc/type_ptr.hpp>

PipelineImGui::PipelineImGui(
	VulkanContext& ctx,
	VkInstance vulkanInstance,
	GLFWwindow* glfwWindow,
	Scene* scene,
	const Camera* camera) :
	PipelineBase(ctx,
		{
			.type_ = PipelineType::GraphicsOnScreen
		}
	),
	scene_(scene),
	camera_(camera)
{
	// Create render pass
	renderPass_.CreateOnScreenColorOnlyRenderPass(ctx);

	// Create framebuffer
	framebuffer_.CreateResizeable(ctx, renderPass_.GetHandle(), {}, IsOffscreen());

	// Create a decsiptor pool
	constexpr uint32_t imageCount = AppConfig::FrameCount;
	VulkanDescriptorInfo dsInfo;
	dsInfo.AddImage(nullptr); // NOTE According to Sascha Willems, we only need one image, if error, then we need to use vkguide.dev code
	descriptor_.CreatePoolAndLayout(ctx, dsInfo, imageCount, 1u, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

	// ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();

	ImGuiStyle* style = &ImGui::GetStyle();
	ImVec4* colors = style->Colors;
	colors[ImGuiCol_WindowBg] = ImVec4(0.0784f, 0.157f, 0.314f, 0.5f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.21f, 0.61f, 0.61f, 1.00f);

	ImGuiIO& io = ImGui::GetIO();
	const std::string filename = AppConfig::FontFolder + "Roboto-Medium.ttf";
	io.Fonts->AddFontFromFileTTF(filename.c_str(), 18.0f);

	// Known issue when using both ImGui and volk
	// github.com/ocornut/imgui/issues/4854
	ImGui_ImplVulkan_LoadFunctions([](const char* functionName, void* vulkanInstance)
		{
			return vkGetInstanceProcAddr(*(reinterpret_cast<VkInstance*>(vulkanInstance)), functionName);
		}, &vulkanInstance);

	ImGui_ImplGlfw_InitForVulkan(glfwWindow, true);

	ImGui_ImplVulkan_InitInfo init_info = {
		.Instance = vulkanInstance,
		.PhysicalDevice = ctx.GetPhysicalDevice(),
		.Device = ctx.GetDevice(),
		.QueueFamily = ctx.GetGraphicsFamily(),
		.Queue = ctx.GetGraphicsQueue(),
		.PipelineCache = nullptr,
		.DescriptorPool = descriptor_.pool_,
		.Subpass = 0,
		.MinImageCount = imageCount,
		.ImageCount = imageCount,
		.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
		.ColorAttachmentFormat = ctx.GetSwapchainImageFormat(),
	};

	ImGui_ImplVulkan_Init(&init_info, renderPass_.GetHandle());
}

PipelineImGui::~PipelineImGui()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void PipelineImGui::ImGuiStart()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void PipelineImGui::ImGuiSetWindow(const char* title, int width, int height, float fontSize)
{
	//ImGui::SetNextWindowSizeConstraints();
	ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width), static_cast<float>(height)));
	ImGui::Begin(title);
	ImGui::SetWindowFontScale(fontSize);
}

void PipelineImGui::ImGuiShowFrameData(FrameCounter* frameCounter)
{
	ImGui::Text("FPS: %.0f", frameCounter->GetCurrentFPS());
	ImGui::Text("Delta: %.0f ms", frameCounter->GetDelayedDeltaMillisecond());
	ImGui::PlotLines("FPS",
		frameCounter->GetGraph(),
		frameCounter->GetGraphLength(),
		0,
		nullptr,
		FLT_MAX,
		FLT_MAX,
		ImVec2(350, 50));
}

void PipelineImGui::ImGuiShowPBRConfig(PushConstPBR* pc, float mipmapCount)
{
	ImGui::SliderFloat("Light Falloff", &(pc->lightFalloff), 0.01f, 5.f);
	ImGui::SliderFloat("Light Intensity", &(pc->lightIntensity), 0.1f, 20.f);
	ImGui::SliderFloat("Albedo", &(pc->albedoMultipler), 0.0f, 1.0f);
	ImGui::SliderFloat("Reflectivity", &(pc->baseReflectivity), 0.01f, 1.f);
	ImGui::SliderFloat("Max Lod", &(pc->maxReflectionLod), 0.1f, mipmapCount);
}

void PipelineImGui::ImGuizmoStart()
{
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::BeginFrame();
	ImGuizmo::SetGizmoSizeClipSpace(0.15);
}

void PipelineImGui::ImGuizmoShowOption(int* editMode)
{
	ImGui::Text("Edit Mode ");
	ImGui::RadioButton("None", editMode, 0); ImGui::SameLine();
	ImGui::RadioButton("Translate", editMode, 1); ImGui::SameLine();
	ImGui::RadioButton("Rotate", editMode, 2); ImGui::SameLine();
	ImGui::RadioButton("Scale", editMode, 3);
}

void PipelineImGui::ImGuizmoShow(glm::mat4& modelMatrix, const int editMode)
{
	if (editMode == EditMode::None)
	{
		return;
	}

	static ImGuizmo::OPERATION gizmoOperation = ImGuizmo::TRANSLATE;

	gizmoOperation = editMode == EditMode::Translate ? ImGuizmo::TRANSLATE :
		(editMode == EditMode::Rotate ? ImGuizmo::ROTATE : ImGuizmo::SCALE);

	ImGuiIO& io = ImGui::GetIO();
	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);

	glm::mat4 view = camera_->GetViewMatrix();
	glm::mat4 projection = camera_->GetProjectionMatrix();
	projection[1][1] *= -1;

	// Note that modelMatrix is modified by the function below
	ImGuizmo::Manipulate(
		glm::value_ptr(view),
		glm::value_ptr(projection),
		gizmoOperation,
		ImGuizmo::WORLD,
		glm::value_ptr(modelMatrix));
}

void PipelineImGui::ImGuizmoManipulateScene(VulkanContext& ctx, InputContext* inputContext)
{
	if (!scene_ || !camera_)
	{
		return;
	}

	if (inputContext->CanSelectObject())
	{
		Ray r = camera_->GetRayFromScreenToWorld(inputContext->mousePositionX, inputContext->mousePositionY);
		int i = scene_->GetClickedInstanceIndex(r);
		if (i >= 0)
		{
			InstanceData& iData = scene_->instanceDataArray_[i];
			inputContext->selectedModelIndex = iData.meshData.modelMatrixIndex_;
			inputContext->selectedInstanceIndex = i;
		}
	}

	if (inputContext->ShowGizmo())
	{
		ImGuizmoStart();
		ImGuizmoShow(
			scene_->modelSSBOs_[inputContext->selectedModelIndex].model,
			inputContext->editMode_);

		// TODO Code smell because UI directly manipulates the scene
		const InstanceData& iData = scene_->instanceDataArray_[inputContext->selectedInstanceIndex];
		scene_->UpdateModelMatrixBuffer(ctx, iData.modelIndex, iData.perModelInstanceIndex);
	}
}

void PipelineImGui::ImGuiEnd()
{
	ImGui::End();
	ImGui::Render();
}

void PipelineImGui::ImGuiDrawEmpty()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::Render();
}

void PipelineImGui::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "ImGui", tracy::Color::DarkSeaGreen);
	const uint32_t swapchainImageIndex = ctx.GetCurrentSwapchainImageIndex();
	ImDrawData* draw_data = ImGui::GetDrawData();
	renderPass_.BeginRenderPass(ctx, commandBuffer, framebuffer_.GetFramebuffer(swapchainImageIndex));
	ctx.InsertDebugLabel(commandBuffer, "PipelineImGui", 0xff9999ff);
	ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
	vkCmdEndRenderPass(commandBuffer);
}