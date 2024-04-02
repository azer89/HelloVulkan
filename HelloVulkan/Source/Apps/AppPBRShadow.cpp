#include "AppPBRShadow.h"
#include "Utility.h"
#include "Configs.h"

#include "PipelineClear.h"
#include "PipelineSkybox.h"
#include "PipelineFinish.h"
#include "PipelineTonemap.h"
#include "PipelineResolveMS.h"

#include "glm/ext.hpp"
#include "imgui_impl_vulkan.h"

AppPBRShadow::AppPBRShadow()
{
}

void AppPBRShadow::Init()
{
	uiData_.shadowCasterPosition_ = { -2.5f, 20.0f, 5.0f };
	camera_->SetPositionAndTarget(glm::vec3(0.0f, 3.0f, 5.0f), glm::vec3(0.0));

	// Init shadow map
	resourcesShadow_ = AddResources<ResourcesShadow>();
	resourcesShadow_->CreateSingleShadowMap(vulkanContext_);

	InitLights();

	// Initialize attachments
	InitSharedResources();

	// Image-Based Lighting
	resourcesIBL_ = AddResources<ResourcesIBL>(vulkanContext_, AppConfig::TextureFolder + "piazza_bologni_1k.hdr");

	std::vector<ModelCreateInfo> dataArray = {
		{
			.filename = AppConfig::ModelFolder + "Sponza/Sponza.gltf",
		},
		{
			.filename = AppConfig::ModelFolder + "Tachikoma/Tachikoma.gltf",
			.clickable = true
		},
		{
			.filename = AppConfig::ModelFolder + "Hexapod/Hexapod.gltf",
			.clickable = true
		}
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
		&(resourcesIBL_->diffuseCubemap_),
		resourcesShared_,
		// This is the first offscreen render pass so we need to clear the color attachment and depth attachment
		RenderPassBit::ColorClear | RenderPassBit::DepthClear);
	shadowPtr_ = AddPipeline<PipelineShadow>(vulkanContext_, scene_.get(), resourcesShadow_);
	// Opaque pass
	pbrOpaquePtr_ = AddPipeline<PipelinePBRShadow>(
		vulkanContext_,
		scene_.get(),
		resourcesLight_,
		resourcesIBL_,
		resourcesShadow_,
		resourcesShared_,
		MaterialType::Opaque);
	// Transparent pass
	pbrTransparentPtr_ = AddPipeline<PipelinePBRShadow>(
		vulkanContext_,
		scene_.get(),
		resourcesLight_,
		resourcesIBL_,
		resourcesShadow_,
		resourcesShared_,
		MaterialType::Transparent);
	lightPtr_ = AddPipeline<PipelineLightRender>(vulkanContext_, resourcesLight_, resourcesShared_);
	// Resolve multiSampledColorImage_ to singleSampledColorImage_
	AddPipeline<PipelineResolveMS>(vulkanContext_, resourcesShared_);
	// This is on-screen render pass that transfers singleSampledColorImage_ to swapchain image
	AddPipeline<PipelineTonemap>(vulkanContext_, &(resourcesShared_->singleSampledColorImage_));
	imguiPtr_ = AddPipeline<PipelineImGui>(vulkanContext_, vulkanInstance_.GetInstance(), glfwWindow_, scene_.get(), camera_.get());
	// Present swapchain image
	AddPipeline<PipelineFinish>(vulkanContext_);
}

void AppPBRShadow::InitLights()
{
	// Lights (SSBO)
	resourcesLight_ = AddResources<ResourcesLight>();
	resourcesLight_->AddLights(vulkanContext_,
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

void AppPBRShadow::UpdateUI()
{
	if (!ShowImGui())
	{
		imguiPtr_->ImGuiDrawEmpty();
		return;
	}

	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("Shadow Mapping", 450, 700);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);

	ImGui::Text("Triangle Count: %i", scene_->triangleCount_);
	ImGui::Checkbox("Render Lights", &uiData_.renderLights_);
	imguiPtr_->ImGuizmoShowOption(&uiData_.gizmoMode_);
	ImGui::SeparatorText("Shading");
	imguiPtr_->ImGuiShowPBRConfig(&uiData_.pbrPC_, resourcesIBL_->cubemapMipmapCount_);

	ImGui::SeparatorText("Shadow mapping");
	ImGui::SliderFloat("Min Bias", &uiData_.shadowMinBias_, 0.f, 0.01f);
	ImGui::SliderFloat("Max Bias", &uiData_.shadowMaxBias_, 0.f, 0.01f);
	ImGui::SliderFloat("Near Plane", &uiData_.shadowNearPlane_, 0.1f, 50.0f);
	ImGui::SliderFloat("Far Plane", &uiData_.shadowFarPlane_, 10.0f, 150.0f);
	ImGui::SliderFloat("Ortho Size", &uiData_.shadowOrthoSize_, 10.0f, 30.0f);

	ImGui::SeparatorText("Light position");
	ImGui::SliderFloat("X", &(uiData_.shadowCasterPosition_[0]), -10.0f, 10.0f);
	ImGui::SliderFloat("Y", &(uiData_.shadowCasterPosition_[1]), 15.0f, 60.0f);
	ImGui::SliderFloat("Z", &(uiData_.shadowCasterPosition_[2]), -10.0f, 10.0f);

	imguiPtr_->ImGuizmoManipulateScene(vulkanContext_, &uiData_);

	imguiPtr_->ImGuiEnd();

	for (auto& pipeline : pipelines_)
	{
		pipeline->UpdateFromIUData(vulkanContext_, uiData_);
	}

	for (auto& resources : resources_)
	{
		resources->UpdateFromUIData(vulkanContext_, uiData_);
	}
}

void AppPBRShadow::UpdateUBOs()
{
	CameraUBO ubo = camera_->GetCameraUBO();
	for (auto& pipeline : pipelines_)
	{
		pipeline->SetCameraUBO(vulkanContext_, ubo);
	}

	shadowPtr_->UpdateShadow(vulkanContext_, resourcesShadow_, resourcesLight_->lights_[0].position_);
	pbrOpaquePtr_->SetShadowMapConfigUBO(vulkanContext_, resourcesShadow_->shadowUBO_);
	pbrTransparentPtr_->SetShadowMapConfigUBO(vulkanContext_, resourcesShadow_->shadowUBO_);
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
	DestroyResources();
}