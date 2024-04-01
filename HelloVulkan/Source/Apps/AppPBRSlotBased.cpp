#include "AppPBRSlotBased.h"
#include "Utility.h"
#include "Configs.h"

#include "PipelineClear.h"
#include "PipelineSkybox.h"
#include "PipelineFinish.h"
#include "PipelineTonemap.h"
#include "PipelineResolveMS.h"
#include "PipelineLightRender.h"
#include "PipelineInfiniteGrid.h"
#include "PipelinePBRSlotBased.h"

#include "glm/ext.hpp"
#include "imgui_impl_vulkan.h"

AppPBRSlotBased::AppPBRSlotBased()
{
}

void AppPBRSlotBased::Init()
{
	camera_->SetPositionAndTarget(glm::vec3(0.0f, 0.5f, 4.0f), glm::vec3(0.0));

	// Initialize lights
	InitLights();

	// Initialize attachments
	InitSharedResources();

	// Image-Based Lighting
	resourcesIBL_ = AddResources<ResourcesIBL>(vulkanContext_, AppConfig::TextureFolder + "piazza_bologni_1k.hdr");

	model_ = std::make_unique<Model>();
	model_->LoadSlotBased(vulkanContext_, AppConfig::ModelFolder + "DamagedHelmet/DamagedHelmet.gltf");
	model_->SetModelUBO(vulkanContext_, { .model = glm::mat4(1.0f) });
	std::vector<Model*> models = { model_.get() };

	// Pipelines have to be created in order
	AddPipeline<PipelineClear>(vulkanContext_); // This is responsible to clear swapchain image
	// This draws a cube
	AddPipeline<PipelineSkybox>(
		vulkanContext_,
		&(resourcesIBL_->environmentCubemap_),
		resourcesShared_,
		// This is the first offscreen render pass so we need to clear the color attachment and depth attachment
		RenderPassBit::ColorClear | RenderPassBit::DepthClear);
	AddPipeline<PipelinePBRSlotBased>(
		vulkanContext_,
		models,
		resourcesLights_,
		resourcesIBL_,
		resourcesShared_);
	AddPipeline<PipelineInfiniteGrid>(vulkanContext_, resourcesShared_, -1.0f);
	AddPipeline<PipelineLightRender>(vulkanContext_, resourcesLights_, resourcesShared_);
	// Resolve multiSampledColorImage_ to singleSampledColorImage_
	AddPipeline<PipelineResolveMS>(vulkanContext_, resourcesShared_);
	// This is on-screen render pass that transfers singleSampledColorImage_ to swapchain image
	AddPipeline<PipelineTonemap>(vulkanContext_, &(resourcesShared_->singleSampledColorImage_));
	imguiPtr_ = AddPipeline<PipelineImGui>(vulkanContext_, vulkanInstance_.GetInstance(), glfwWindow_);
	// Present swapchain image
	AddPipeline<PipelineFinish>(vulkanContext_);
}

void AppPBRSlotBased::InitLights()
{
	// Lights (SSBO)
	resourcesLights_ = AddResources<ResourcesLight>();
	resourcesLights_->AddLights(vulkanContext_,
	{
		{.position_ = glm::vec4(-1.5f, 0.7f,  1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{.position_ = glm::vec4(1.5f, 0.7f,  1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{.position_ = glm::vec4(-1.5f, 0.7f, -1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{.position_ = glm::vec4(1.5f, 0.7f, -1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f }
	});
}

void AppPBRSlotBased::UpdateUBOs()
{
	CameraUBO ubo = camera_->GetCameraUBO();
	for (auto& pipeline : pipelines_)
	{
		pipeline->SetCameraUBO(vulkanContext_, ubo);
	}
}

void AppPBRSlotBased::UpdateUI()
{
	if (!ShowImGui())
	{
		imguiPtr_->ImGuiDrawEmpty();
		return;
	}

	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("PBR and IBL", 450, 350);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);
	ImGui::Checkbox("Render Lights", &uiData_.renderLights_);
	ImGui::Checkbox("Render Grid", &uiData_.renderInfiniteGrid_);
	imguiPtr_->ImGuiShowPBRConfig(&uiData_.pbrPC_, resourcesIBL_->cubemapMipmapCount_);
	imguiPtr_->ImGuiEnd();

	for (auto& pipeline : pipelines_)
	{
		pipeline->UpdateFromIUData(vulkanContext_, uiData_);
	}
}

// This is called from main.cpp
void AppPBRSlotBased::MainLoop()
{
	InitVulkan({
		.supportRaytracing_ = false,
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

	model_->Destroy();
	DestroyResources();
}