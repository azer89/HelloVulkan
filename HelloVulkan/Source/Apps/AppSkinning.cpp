#include "AppSkinning.h"
#include "Utility.h"
#include "Configs.h"

#include "PipelineSkybox.h"
#include "PipelineClear.h"
#include "PipelineFinish.h"
#include "PipelineTonemap.h"
#include "PipelineResolveMS.h"
#include "PipelineSkinning.h"

#include "glm/ext.hpp"
#include "imgui_impl_vulkan.h"

#include <iostream>

AppSkinning::AppSkinning()
{
}

void AppSkinning::Init()
{
	uiData_.shadowMinBias_ = 0.003f;
	uiData_.shadowCasterPosition_ = { -0.5f, 20.0f, 5.0f };
	camera_->SetPositionAndTarget(glm::vec3(0.0f, 3.0f, 5.0f), glm::vec3(0.0));

	// Init shadow map
	resourcesShadow_ = AddResources<ResourcesShadow>();
	resourcesShadow_->CreateSingleShadowMap(vulkanContext_);

	resourcesGBuffer_ = AddResources<ResourcesGBuffer>();
	resourcesGBuffer_->Create(vulkanContext_);

	InitLights();

	// Initialize attachments
	InitSharedResources();

	// Image-Based Lighting
	resourcesIBL_ = AddResources<ResourcesIBL>(vulkanContext_, AppConfig::TextureFolder + "piazza_bologni_1k.hdr");

	InitScene();

	// Pipelines
	AddPipeline<PipelineClear>(vulkanContext_); // This is responsible to clear swapchain image
	// This draws a cube
	AddPipeline<PipelineSkybox>(
		vulkanContext_,
		&(resourcesIBL_->diffuseCubemap_),
		resourcesShared_,
		// This is the first offscreen render pass so we need to clear the color attachment and depth attachment
		RenderPassBit::ColorClear | RenderPassBit::DepthClear);
	AddPipeline<PipelineSkinning>(vulkanContext_, scene_.get());
	gPtr_ = AddPipeline<PipelineGBuffer>(vulkanContext_, scene_.get(), resourcesGBuffer_);
	ssaoPtr_ = AddPipeline<PipelineSSAO>(vulkanContext_, resourcesGBuffer_);
	shadowPtr_ = AddPipeline<PipelineShadow>(vulkanContext_, scene_.get(), resourcesShadow_);
	// Opaque pass
	pbrOpaquePtr_ = AddPipeline<PipelinePBRShadow>(
		vulkanContext_,
		scene_.get(),
		resourcesLight_,
		resourcesIBL_,
		resourcesShadow_,
		resourcesShared_,
		resourcesGBuffer_,
		MaterialType::Opaque);
	// Transparent pass
	pbrTransparentPtr_ = AddPipeline<PipelinePBRShadow>(
		vulkanContext_,
		scene_.get(),
		resourcesLight_,
		resourcesIBL_,
		resourcesShadow_,
		resourcesShared_,
		resourcesGBuffer_,
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

void AppSkinning::InitScene()
{
	// Scene
	std::vector<ModelCreateInfo> dataArray = 
	{{
		.filename = AppConfig::ModelFolder + "Sponza/Sponza.gltf"
	},
	{
		.filename = AppConfig::ModelFolder + "DancingStormtrooper01/DancingStormtrooper01.gltf",
		.instanceCount = 4,
		.playAnimation = true,
		.clickable = true
	},
	{
		.filename = AppConfig::ModelFolder + "DancingStormtrooper02/DancingStormtrooper02.gltf",
		.instanceCount = 4,
		.playAnimation = true,
		.clickable = true
	}};
	scene_ = std::make_unique<Scene>(vulkanContext_, dataArray);

	constexpr uint32_t xCount = 2;
	constexpr uint32_t zCount = 4;
	constexpr float dist = 1.2f;
	constexpr float xMidPos = static_cast<float>(xCount - 1) * dist / 2.0f;
	constexpr float zMidPos = static_cast<float>(zCount - 1) * dist / 2.0f;
	constexpr float yPos = -0.33f;
	const glm::vec3 scale = glm::vec3(0.5f, 0.5f, 0.5f);

	for (uint32_t x = 0; x < xCount; ++x)
	{
		for (uint32_t z = 0; z < zCount; ++z)
		{
			float xPos = static_cast<float>(x) * dist - xMidPos;
			float zPos = -(static_cast<float>(z) * dist) + zMidPos;
			glm::mat4 modelMatrix(1.f);
			modelMatrix = glm::translate(modelMatrix, glm::vec3(xPos, yPos, zPos));
			modelMatrix = glm::scale(modelMatrix, scale);
			uint32_t modelIndex = x % 2;
			uint32_t instanceIndex = z; // May not work if you change xCount and zCount
			scene_->UpdateModelMatrix(vulkanContext_, { .model = modelMatrix }, modelIndex + 1, instanceIndex);
		}
	}
}

void AppSkinning::InitLights()
{
	// Lights (SSBO)
	resourcesLight_ = AddResources<ResourcesLight>();
	resourcesLight_->AddLights(vulkanContext_,
	{
		// The first light is a shadow caster
		{.color_ = glm::vec4(1.f), .radius_ = 1.0f },

		// Add additional lights so that the scene is not too dark
		{.position_ = glm::vec4(-1.5f, 2.5f,  5.f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{.position_ = glm::vec4(1.5f, 2.5f,  5.f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{.position_ = glm::vec4(-1.5f, 2.5f, -5.f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{.position_ = glm::vec4(1.5f, 2.5f, -5.f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f }
	});
}

void AppSkinning::UpdateUI()
{
	if (!ShowImGui())
	{
		imguiPtr_->ImGuiDrawEmpty();
		return;
	}

	// Start
	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("Compute-based Skinning", 450, 750);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);

	ImGui::Text("Triangle Count: %i", scene_->triangleCount_);
	ImGui::Checkbox("Render Lights", &uiData_.renderLights_);
	imguiPtr_->ImGuizmoShowOption(&uiData_.gizmoMode_);
	ImGui::SeparatorText("Shading");
	imguiPtr_->ImGuiShowPBRConfig(&uiData_.pbrPC_, resourcesIBL_->cubemapMipmapCount_);

	ImGui::SeparatorText("Shadow map");
	ImGui::SliderFloat("Min Bias", &uiData_.shadowMinBias_, 0.f, 0.01f);
	ImGui::SliderFloat("Max Bias", &uiData_.shadowMaxBias_, 0.f, 0.01f);
	ImGui::SliderFloat("Near Plane", &uiData_.shadowNearPlane_, 0.1f, 50.0f);
	ImGui::SliderFloat("Far Plane", &uiData_.shadowFarPlane_, 10.0f, 150.0f);
	ImGui::SliderFloat("Ortho Size", &uiData_.shadowOrthoSize_, 10.0f, 100.0f);

	ImGui::SeparatorText("Shadow caster");
	ImGui::SliderFloat("X", &(uiData_.shadowCasterPosition_[0]), -10.0f, 10.0f);
	ImGui::SliderFloat("Y", &(uiData_.shadowCasterPosition_[1]), 5.0f, 60.0f);
	ImGui::SliderFloat("Z", &(uiData_.shadowCasterPosition_[2]), -10.0f, 10.0f);

	ImGui::SeparatorText("SSAO");
	ImGui::SliderFloat("Radius", &uiData_.ssaoRadius_, 0.0f, 10.0f);
	ImGui::SliderFloat("Bias", &uiData_.ssaoBias_, 0.0f, 0.5f);
	ImGui::SliderFloat("Power", &uiData_.ssaoPower_, 0.1f, 5.0f);

	imguiPtr_->ImGuizmoManipulateScene(vulkanContext_, &uiData_);
	
	// End
	imguiPtr_->ImGuiEnd();

	for (auto& pipeline : pipelines_)
	{
		pipeline->UpdateFromUIData(vulkanContext_, uiData_);
	}
	for (auto& resources : resources_)
	{
		resources->UpdateFromUIData(vulkanContext_, uiData_);
	}
}

void AppSkinning::UpdateUBOs()
{
	scene_->UpdateAnimation(vulkanContext_, frameCounter_.GetDeltaSecond());

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
void AppSkinning::MainLoop()
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

	DestroyResources();
}