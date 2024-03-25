#include "AppPBRShadow.h"
#include "VulkanUtility.h"
#include "Configs.h"

#include "glm/ext.hpp"
#include "imgui_impl_vulkan.h"

AppPBRShadow::AppPBRShadow()
{
}

void AppPBRShadow::Init()
{
	camera_->SetPositionAndTarget(glm::vec3(0.0f, 3.0f, 5.0f), glm::vec3(0.0));

	// Init shadow map
	resShadow_ = AddResources<ResourcesShadow>();
	resShadow_->CreateSingleShadowMap(vulkanContext_);

	InitLights();

	// Initialize attachments
	InitSharedResources2();

	// Image-Based Lighting
	resIBL2_ = AddResources<ResourcesIBL>(vulkanContext_, AppConfig::TextureFolder + "piazza_bologni_1k.hdr");

	std::vector<ModelCreateInfo> dataArray = {
		{AppConfig::ModelFolder + "Sponza/Sponza.gltf", 1},
		{AppConfig::ModelFolder + "Tachikoma/Tachikoma.gltf", 1},
		{AppConfig::ModelFolder + "Hexapod/Hexapod.gltf", 1}
	};
	bool supportDeviceAddress = true;
	scene_ = std::make_unique<Scene>(vulkanContext_, dataArray, supportDeviceAddress);

	// Model matrix for Tachikoma
	glm::mat4 modelMatrix(1.f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.15f, 0.35f, 1.5f));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(45.f), glm::vec3(0.f, 1.f, 0.f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(0.7f, 0.7f, 0.7f));
	scene_->UpdateModelMatrix(vulkanContext_, { .model = modelMatrix }, 1, 0);

	// Model matrix for Hexapod
	modelMatrix = glm::mat4(1.f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.62f, -1.5f));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f));
	scene_->UpdateModelMatrix(vulkanContext_, { .model = modelMatrix }, 2, 0);

	// Pipelines
	AddPipeline<PipelineClear>(vulkanContext_); // This is responsible to clear swapchain image
	AddPipeline<PipelineSkybox>(
		vulkanContext_,
		&(resIBL2_->diffuseCubemap_),
		resShared2_,
		// This is the first offscreen render pass so we need to clear the color attachment and depth attachment
		RenderPassBit::ColorClear | RenderPassBit::DepthClear);
	shadowPtr_ = AddPipeline<PipelineShadow>(vulkanContext_, scene_.get(), resShadow_);
	// Opaque pass
	pbrOpaquePtr_ = AddPipeline<PipelinePBRShadow>(
		vulkanContext_,
		scene_.get(),
		resLight_,
		resIBL2_,
		resShadow_,
		resShared2_,
		MaterialType::Opaque);
	// Transparent pass
	pbrTransparentPtr_ = AddPipeline<PipelinePBRShadow>(
		vulkanContext_,
		scene_.get(),
		resLight_,
		resIBL2_,
		resShadow_,
		resShared2_,
		MaterialType::Transparent);
	lightPtr_ = AddPipeline<PipelineLightRender>(vulkanContext_, resLight_, resShared2_);
	// Resolve multiSampledColorImage_ to singleSampledColorImage_
	AddPipeline<PipelineResolveMS>(vulkanContext_, resShared2_);
	// This is on-screen render pass that transfers singleSampledColorImage_ to swapchain image
	AddPipeline<PipelineTonemap>(vulkanContext_, &(resShared2_->singleSampledColorImage_));
	imguiPtr_ = AddPipeline<PipelineImGui>(vulkanContext_, vulkanInstance_.GetInstance(), glfwWindow_);
	// Present swapchain image
	AddPipeline<PipelineFinish>(vulkanContext_);

	// ImGui
	inputContext_.shadowCasterPosition_ = { -2.5f, 20.0f, 5.0f };
}

void AppPBRShadow::InitLights()
{
	// Lights (SSBO)
	resLight_ = AddResources<ResourcesLight>();
	resLight_->AddLights(vulkanContext_,
	{
		// The first light is used to generate the shadow map
		// and its position is set by ImGui
		{.color_ = glm::vec4(1.f), .radius_ = 1.0f },

		// Add additional lights so that the scene is not too dark
		{.position_ = glm::vec4(-1.5f, 2.5f,  5.f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{.position_ = glm::vec4(1.5f, 2.5f,  5.f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{.position_ = glm::vec4(-1.5f, 2.5f, -5.f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{.position_ = glm::vec4(1.5f, 2.5f, -5.f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f }
	});
}

void AppPBRShadow::UpdateUBOs()
{
	CameraUBO ubo = camera_->GetCameraUBO();
	for (auto& pipeline : pipelines2_)
	{
		pipeline->SetCameraUBO(vulkanContext_, ubo);
	}

	shadowPtr_->UpdateShadow(vulkanContext_, resShadow_, resLight_->lights_[0].position_);
	pbrOpaquePtr_->SetShadowMapConfigUBO(vulkanContext_, resShadow_->shadowUBO_);
	pbrTransparentPtr_->SetShadowMapConfigUBO(vulkanContext_, resShadow_->shadowUBO_);
}

void AppPBRShadow::UpdateUI()
{
	if (!showImgui_)
	{
		imguiPtr_->ImGuiDrawEmpty();
		return;
	}

	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("Shadow Mapping", 500, 650);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);

	ImGui::Text("Triangle Count: %i", scene_->triangleCount_);
	ImGui::Checkbox("Render Lights", &inputContext_.renderLights_);
	ImGui::SeparatorText("Shading");
	imguiPtr_->ImGuiShowPBRConfig(&inputContext_.pbrPC_, resIBL2_->cubemapMipmapCount_);

	ImGui::SeparatorText("Shadow mapping");
	ImGui::SliderFloat("Min Bias", &inputContext_.shadowMinBias_, 0.f, 0.01f);
	ImGui::SliderFloat("Max Bias", &inputContext_.shadowMaxBias_, 0.f, 0.01f);
	ImGui::SliderFloat("Near Plane", &inputContext_.shadowNearPlane_, 0.1f, 50.0f);
	ImGui::SliderFloat("Far Plane", &inputContext_.shadowFarPlane_, 10.0f, 150.0f);
	ImGui::SliderFloat("Ortho Size", &inputContext_.shadowOrthoSize_, 10.0f, 30.0f);

	ImGui::SeparatorText("Light position");
	ImGui::SliderFloat("X", &(inputContext_.shadowCasterPosition_[0]), -10.0f, 10.0f);
	ImGui::SliderFloat("Y", &(inputContext_.shadowCasterPosition_[1]), 15.0f, 60.0f);
	ImGui::SliderFloat("Z", &(inputContext_.shadowCasterPosition_[2]), -10.0f, 10.0f);

	imguiPtr_->ImGuiEnd();

	// TODO Check if light position is changed 
	resLight_->UpdateLightPosition(vulkanContext_, 0, &(inputContext_.shadowCasterPosition_[0]));

	lightPtr_->ShouldRender(inputContext_.renderLights_);
	pbrOpaquePtr_->SetPBRPushConstants(inputContext_.pbrPC_);
	pbrTransparentPtr_->SetPBRPushConstants(inputContext_.pbrPC_);

	resShadow_->shadowUBO_.shadowMinBias = inputContext_.shadowMinBias_;
	resShadow_->shadowUBO_.shadowMaxBias = inputContext_.shadowMaxBias_;
	resShadow_->shadowNearPlane_ = inputContext_.shadowNearPlane_;
	resShadow_->shadowFarPlane_ = inputContext_.shadowFarPlane_;
	resShadow_->orthoSize_ = inputContext_.shadowOrthoSize_;
}

// This is called from main.cpp
void AppPBRShadow::MainLoop()
{
	InitVulkan({
		.suportBufferDeviceAddress_ = true,
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
	DestroyInternal();
}