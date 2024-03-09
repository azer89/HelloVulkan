#include "AppSimpleRaytracing.h"
#include "Scene.h"

#include "imgui_impl_volk.h"

AppSimpleRaytracing::AppSimpleRaytracing()
{
}

void AppSimpleRaytracing::Init()
{
	// Initialize attachments
	InitSharedResources();

	// Scene
	std::vector<std::string> modelFiles = {
		AppConfig::ModelFolder + "Dragon//Dragon.obj"
	};
	scene_ = std::make_unique<Scene>(vulkanContext_, modelFiles);

	clearPtr_ = std::make_unique<PipelineClear>(vulkanContext_);
	rtxPtr_ = std::make_unique<PipelineSimpleRaytracing>(vulkanContext_, scene_.get());
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
		imguiPtr_->ImGuiDrawEmpty();
		return;
	}

	imguiPtr_->ImGuiStart();
	ImGui::SetNextWindowSize(ImVec2(525, 250));
	imguiPtr_->ImGuiSetWindow("Raytracing", 525, 200);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);
	imguiPtr_->ImGuiEnd();
}

void AppSimpleRaytracing::DestroyResources()
{
	scene_.reset();

	clearPtr_.reset();
	finishPtr_.reset();
	imguiPtr_.reset();
	rtxPtr_.reset();
}

void AppSimpleRaytracing::MainLoop()
{
	InitVulkan({
		.supportRaytracing_ = true,
		.supportMSAA_ = true
		});

	Init();

	// Main loop
	while (StillRunning())
	{
		PollEvents();
		ProcessTiming();
		ProcessInput();
		DrawFrame();
	}

	DestroyResources();
	DestroyInternal();
}
