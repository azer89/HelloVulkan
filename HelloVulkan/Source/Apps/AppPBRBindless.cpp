#include "AppPBRBindless.h"
#include "VulkanUtility.h"
#include "Configs.h"

#include "glm/ext.hpp"
#include "imgui_impl_vulkan.h"

AppPBRBindless::AppPBRBindless()
{
}

void AppPBRBindless::Init()
{
	camera_->SetPositionAndTarget(glm::vec3(0.0f, 3.0f, 5.0f), glm::vec3(0.0));

	InitLights();

	// Initialize attachments
	InitSharedResources2();

	// Image-Based Lighting
	resIBL2_ = AddResources<ResourcesIBL>(vulkanContext_, AppConfig::TextureFolder + "piazza_bologni_1k.hdr");

	// Scene
	std::vector<ModelCreateInfo> dataArray = { 
		{ AppConfig::ModelFolder + "Sponza/Sponza.gltf", 1},
		{ AppConfig::ModelFolder + "Tachikoma/Tachikoma.gltf", 1},
	};
	bool supportDeviceAddress = true;
	scene_ = std::make_unique<Scene>(vulkanContext_, dataArray, supportDeviceAddress);

	// Tachikoma model matrix
	glm::mat4 modelMatrix(1.f);
	modelMatrix = glm::rotate(modelMatrix, glm::radians(45.f), glm::vec3(0.f, 1.f, 0.f));
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.5f, 0.62f, 0.f));
	scene_->UpdateModelMatrix(vulkanContext_, { .model = modelMatrix }, 1, 0);

	// Pipelines
	AddPipeline<PipelineClear>(vulkanContext_); // This is responsible to clear swapchain image
	// This draws a cube
	AddPipeline<PipelineSkybox>(
		vulkanContext_,
		&(resIBL2_->diffuseCubemap_),
		resShared2_,
		// This is the first offscreen render pass so we need to clear the color attachment and depth attachment
		RenderPassBit::ColorClear | RenderPassBit::DepthClear);
	pbrPtr_ = AddPipeline<PipelinePBRBindless>(
		vulkanContext_,
		scene_.get(),
		resourcesLight_,
		resIBL2_,
		resShared2_);
	lightPtr_ = AddPipeline<PipelineLightRender>(
		vulkanContext_,
		resourcesLight_,
		resShared2_);
	// Resolve multiSampledColorImage_ to singleSampledColorImage_
	AddPipeline<PipelineResolveMS>(vulkanContext_, resShared2_);
	// This is on-screen render pass that transfers singleSampledColorImage_ to swapchain image
	AddPipeline<PipelineTonemap>(vulkanContext_, &(resShared2_->singleSampledColorImage_));
	imguiPtr_ = AddPipeline<PipelineImGui>(vulkanContext_, vulkanInstance_.GetInstance(), glfwWindow_);
	// Present swapchain image
	AddPipeline<PipelineFinish>(vulkanContext_);
}

void AppPBRBindless::InitLights()
{
	// Lights (SSBO)
	resourcesLight_ = AddResources<ResourcesLight>();
	resourcesLight_->AddLights(vulkanContext_,
	{
		{ .position_ = glm::vec4(-1.5f, 0.7f,  1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{ .position_ = glm::vec4(1.5f, 0.7f,  1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{ .position_ = glm::vec4(-1.5f, 0.7f, -1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{ .position_ = glm::vec4(1.5f, 0.7f, -1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f }
	});
}

void AppPBRBindless::UpdateUBOs()
{
	CameraUBO ubo = camera_->GetCameraUBO();
	for (auto& pipeline : pipelines2_)
	{
		pipeline->SetCameraUBO(vulkanContext_, ubo);
	}
}

void AppPBRBindless::UpdateUI()
{
	if (!showImgui_)
	{
		imguiPtr_->ImGuiDrawEmpty();
		return;
	}

	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("Bindless Textures", 500, 350);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);
	ImGui::Text("Triangle Count: %i", scene_->triangleCount_);
	ImGui::Checkbox("Render Lights", &inputContext_.renderLights_);
	imguiPtr_->ImGuiShowPBRConfig(&inputContext_.pbrPC_, resIBL2_->cubemapMipmapCount_);
	imguiPtr_->ImGuiEnd();

	lightPtr_->ShouldRender(inputContext_.renderLights_);
	pbrPtr_->SetPBRPushConstants(inputContext_.pbrPC_);
}

// This is called from main.cpp
void AppPBRBindless::MainLoop()
{
	InitVulkan({
		.suportBufferDeviceAddress_ = true,
		.supportMSAA_ = true,
		.supportBindlessTextures_ = true,
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

	DestroyInternal();
}