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
	InitSharedResources();

	// Image-Based Lighting
	resIBL_ = std::make_unique<ResourcesIBL>(vulkanContext_, AppConfig::TextureFolder + "piazza_bologni_1k.hdr");

	model_ = std::make_unique<Model>();
	model_->LoadSlotBased(vulkanContext_, AppConfig::ModelFolder + "DamagedHelmet/DamagedHelmet.gltf");
	model_->SetModelUBO(vulkanContext_, { .model = glm::mat4(1.0f) });
	std::vector<Model*> models = { model_.get() };

	// Pipelines
	AddPipeline<PipelineClear>(vulkanContext_); // This is responsible to clear swapchain image
	// This draws a cube
	skyboxPtr_ = AddPipeline<PipelineSkybox>(
		vulkanContext_,
		&(resIBL_->environmentCubemap_),
		resShared_.get(),
		// This is the first offscreen render pass so we need to clear the color attachment and depth attachment
		RenderPassBit::ColorClear | RenderPassBit::DepthClear);
	pbrPtr_ = AddPipeline<PipelinePBRSlotBased>(
		vulkanContext_,
		models,
		resLights_.get(),
		resIBL_.get(),
		resShared_.get());
	infGridPtr_ = AddPipeline<PipelineInfiniteGrid>(vulkanContext_, resShared_.get(), -1.0f);
	lightPtr_ =  AddPipeline<PipelineLightRender>(vulkanContext_, resLights_.get(), resShared_.get());
	// Resolve multiSampledColorImage_ to singleSampledColorImage_
	AddPipeline<PipelineResolveMS>(vulkanContext_, resShared_.get());
	// This is on-screen render pass that transfers singleSampledColorImage_ to swapchain image
	AddPipeline<PipelineTonemap>(vulkanContext_, &(resShared_->singleSampledColorImage_));
	imguiPtr_ = AddPipeline<PipelineImGui>(vulkanContext_, vulkanInstance_.GetInstance(), glfwWindow_);
	// Present swapchain image
	AddPipeline<PipelineFinish>(vulkanContext_);

	// Put all renderer pointers to a vector
	/*pipelines_ =
	{
		// Must be in order
		clearPtr_.get(),
		skyboxPtr_.get(),
		pbrPtr_.get(),
		infGridPtr_.get(),
		lightPtr_.get(),
		resolveMSPtr_.get(),
		tonemapPtr_.get(),
		imguiPtr_.get(),
		finishPtr_.get()
	};*/
}

void AppPBRSlotBased::InitLights()
{
	// Lights (SSBO)
	resLights_ = std::make_unique<ResourcesLight>();
	resLights_->AddLights(vulkanContext_,
		{
			{.position_ = glm::vec4(-1.5f, 0.7f,  1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
			{.position_ = glm::vec4(1.5f, 0.7f,  1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
			{.position_ = glm::vec4(-1.5f, 0.7f, -1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
			{.position_ = glm::vec4(1.5f, 0.7f, -1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f }
		});
}

void AppPBRSlotBased::DestroyResources()
{
	// Resources
	resIBL_.reset();
	resLights_.reset();

	// Destroy meshes
	model_->Destroy();
	model_.reset();
	
	// Destroy renderers
	/*clearPtr_.reset();
	finishPtr_.reset();
	skyboxPtr_.reset();
	pbrPtr_.reset();
	lightPtr_.reset();
	resolveMSPtr_.reset();
	tonemapPtr_.reset();
	imguiPtr_.reset();
	infGridPtr_.reset();*/
}

void AppPBRSlotBased::UpdateUBOs()
{
	CameraUBO ubo = camera_->GetCameraUBO();
	/*for (auto& pipeline : pipelines2_)
	{
		pipeline->SetCameraUBO(vulkanContext_, ubo);
	}*/

	lightPtr_->SetCameraUBO(vulkanContext_, ubo);
	pbrPtr_->SetCameraUBO(vulkanContext_, ubo);
	infGridPtr_->SetCameraUBO(vulkanContext_, ubo);
	skyboxPtr_->SetCameraUBO(vulkanContext_, ubo);
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
	imguiPtr_->ImGuiShowPBRConfig(&inputContext_.pbrPC_, resIBL_->cubemapMipmapCount_);
	
	imguiPtr_->ImGuiEnd();

	lightPtr_->ShouldRender(inputContext_.renderLights_);
	infGridPtr_->ShouldRender(inputContext_.renderInfiniteGrid_);
	pbrPtr_->SetPBRPushConstants(inputContext_.pbrPC_);
	/*for (auto& pipeline : pipelines2_)
	{
		pipeline->GetUpdateFromInputContext(vulkanContext_, inputContext_);
	}*/
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

	DestroyResources();
	DestroyInternal();
}