#include "AppSimpleRaytracing.h"

#include "Configs.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_volk.h"

AppSimpleRaytracing::AppSimpleRaytracing()
{
}

void AppSimpleRaytracing::Init()
{
	// Initialize attachments
	InitSharedImageResources();
	clearPtr_ = std::make_unique<PipelineClear>(vulkanContext_);
	rtxPtr_ = std::make_unique<PipelineSimpleRaytracing>(vulkanContext_);
	imguiPtr_ = std::make_unique<PipelineImGui>(vulkanContext_, vulkanInstance_.GetInstance(), glfwWindow_);
	finishPtr_ = std::make_unique<PipelineFinish>(vulkanContext_);

	pipelines_ =
	{
		// Must be in order
		clearPtr_.get(),
		rtxPtr_.get(),
		imguiPtr_.get(),
		finishPtr_.get()
	};
}

void AppSimpleRaytracing::UpdateUBOs()
{
	rtxPtr_->SetRaytracingCameraUBO(vulkanContext_, camera_->GetRaytracingCameraUBO());
}

void AppSimpleRaytracing::UpdateUI()
{
	if (!showImgui_)
	{
		imguiPtr_->DrawEmptyImGui();
		return;
	}

	imguiPtr_->StartImGui();

	ImGui::SetNextWindowSize(ImVec2(525, 250));
	ImGui::Begin("Raytracing");
	ImGui::SetWindowFontScale(1.25f);

	imguiPtr_->EndImGui();
}

void AppSimpleRaytracing::DestroyResources()
{
	clearPtr_.reset();
	finishPtr_.reset();
	imguiPtr_.reset();
	rtxPtr_.reset();
}

int AppSimpleRaytracing::MainLoop()
{
	InitVulkan({
		.supportRaytracing_ = true,
		.supportMSAA_ = true
		});

	Init();

	// Main loop
	while (!GLFWWindowShouldClose())
	{
		PollEvents();
		ProcessTiming();
		ProcessInput();
		DrawFrame();
	}

	// Wait until everything is finished
	vkDeviceWaitIdle(vulkanContext_.GetDevice());

	DestroyResources();
	Terminate();

	return 0;
}
