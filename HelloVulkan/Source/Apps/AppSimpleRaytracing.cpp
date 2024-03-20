#include "AppSimpleRaytracing.h"

#include "imgui_impl_vulkan.h"

AppSimpleRaytracing::AppSimpleRaytracing()
{
}

void AppSimpleRaytracing::Init()
{
	camera_->SetPositionAndTarget(glm::vec3(0.0f, 3.0f, 5.0f), glm::vec3(0.0f, 0.0f, -3.5f));

	// Initialize attachments
	InitSharedResources();

	// Scene
	std::vector<ModelCreateInfo> dataArray = {
		{ AppConfig::ModelFolder + "Dragon/Dragon.obj", 1}
	};
	scene_ = std::make_unique<Scene>(vulkanContext_, dataArray);

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
	imguiPtr_->ImGuiSetWindow("Raytracing", 525, 150);
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
