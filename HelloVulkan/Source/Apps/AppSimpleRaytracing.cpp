#include "AppSimpleRaytracing.h"

#include "imgui_impl_vulkan.h"

AppSimpleRaytracing::AppSimpleRaytracing()
{
}

void AppSimpleRaytracing::Init()
{
	camera_->SetPositionAndTarget(glm::vec3(0.0f, 3.0f, 5.0f), glm::vec3(0.0f, 0.0f, -3.5f));

	// Scene
	std::vector<ModelCreateInfo> dataArray = {
		{ AppConfig::ModelFolder + "Dragon/Dragon.obj", 1}
	};
	scene_ = std::make_unique<Scene>(vulkanContext_, dataArray);

	AddPipeline<PipelineClear>(vulkanContext_);
	rtxPtr_ = AddPipeline<PipelineSimpleRaytracing>(vulkanContext_, scene_.get());
	imguiPtr_ = AddPipeline<PipelineImGui>(vulkanContext_, vulkanInstance_.GetInstance(), glfwWindow_);
	AddPipeline<PipelineFinish>(vulkanContext_);
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
	imguiPtr_->ImGuiSetWindow("Raytracing", 500, 150);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);
	imguiPtr_->ImGuiEnd();
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

	scene_.reset();
	DestroyResources();
}
