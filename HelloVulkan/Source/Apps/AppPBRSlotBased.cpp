#include "AppPBRSlotBased.h"
#include "VulkanUtility.h"
#include "Configs.h"

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
	InitSharedResources2();

	// Image-Based Lighting
	resIBL2_ = AddResources<ResourcesIBL>(vulkanContext_, AppConfig::TextureFolder + "piazza_bologni_1k.hdr");

	model_ = std::make_unique<Model>();
	model_->LoadSlotBased(vulkanContext_, AppConfig::ModelFolder + "DamagedHelmet/DamagedHelmet.gltf");
	model_->SetModelUBO(vulkanContext_, { .model = glm::mat4(1.0f) });
	std::vector<Model*> models = { model_.get() };

	// Pipelines have to be created in order
	AddPipeline<PipelineClear>(vulkanContext_); // This is responsible to clear swapchain image
	// This draws a cube
	AddPipeline<PipelineSkybox>(
		vulkanContext_,
		&(resIBL2_->environmentCubemap_),
		resShared2_,
		// This is the first offscreen render pass so we need to clear the color attachment and depth attachment
		RenderPassBit::ColorClear | RenderPassBit::DepthClear);
	AddPipeline<PipelinePBRSlotBased>(
		vulkanContext_,
		models,
		resLights_,
		resIBL2_,
		resShared2_);
	AddPipeline<PipelineInfiniteGrid>(vulkanContext_, resShared2_, -1.0f);
	AddPipeline<PipelineLightRender>(vulkanContext_, resLights_, resShared2_);
	// Resolve multiSampledColorImage_ to singleSampledColorImage_
	AddPipeline<PipelineResolveMS>(vulkanContext_, resShared2_);
	// This is on-screen render pass that transfers singleSampledColorImage_ to swapchain image
	AddPipeline<PipelineTonemap>(vulkanContext_, &(resShared2_->singleSampledColorImage_));
	imguiPtr_ = AddPipeline<PipelineImGui>(vulkanContext_, vulkanInstance_.GetInstance(), glfwWindow_);
	// Present swapchain image
	AddPipeline<PipelineFinish>(vulkanContext_);
}

void AppPBRSlotBased::InitLights()
{
	// Lights (SSBO)
	resLights_ = AddResources<ResourcesLight>();
	resLights_->AddLights(vulkanContext_,
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
	for (auto& pipeline : pipelines2_)
	{
		pipeline->SetCameraUBO(vulkanContext_, ubo);
	}
}

void AppPBRSlotBased::UpdateUI()
{
	if (!showImgui_)
	{
		imguiPtr_->ImGuiDrawEmpty();
		return;
	}

	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("PBR and IBL", 500, 325);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);
	ImGui::Checkbox("Render Lights", &inputContext_.renderLights_);
	ImGui::Checkbox("Render Grid", &inputContext_.renderInfiniteGrid_);
	imguiPtr_->ImGuiShowPBRConfig(&inputContext_.pbrPC_, resIBL2_->cubemapMipmapCount_);
	imguiPtr_->ImGuiEnd();

	for (auto& pipeline : pipelines2_)
	{
		pipeline->GetUpdateFromInputContext(vulkanContext_, inputContext_);
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
	DestroyInternal();
}