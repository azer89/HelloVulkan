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

AppSkinning::AppSkinning()
{
}

void AppSkinning::Init()
{
	inputContext_.shadowMinBias_ = 0.003f;
	inputContext_.shadowCasterPosition_ = { -0.5f, 20.0f, 5.0f };
	camera_->SetPositionAndTarget(glm::vec3(0.0f, 3.0f, 5.0f), glm::vec3(0.0));

	// Init shadow map
	resourcesShadow_ = AddResources<ResourcesShadow>();
	resourcesShadow_->CreateSingleShadowMap(vulkanContext_);

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
	imguiPtr_ = AddPipeline<PipelineImGui>(vulkanContext_, vulkanInstance_.GetInstance(), glfwWindow_);
	// Present swapchain image
	AddPipeline<PipelineFinish>(vulkanContext_);
}

void AppSkinning::InitScene()
{
	// Scene
	std::vector<ModelCreateInfo> dataArray = {
		{
			.filename = AppConfig::ModelFolder + "Sponza/Sponza.gltf",
			.instanceCount = 1,
			.playAnimation = false
		},
		{
			.filename = AppConfig::ModelFolder + "DancingStormtrooper01/DancingStormtrooper01.gltf",
			.instanceCount = 4,
			.playAnimation = true
		},
		{
			.filename = AppConfig::ModelFolder + "DancingStormtrooper02/DancingStormtrooper02.gltf",
			.instanceCount = 4,
			.playAnimation = true
		}
	};
	bool supportDeviceAddress = true;
	scene_ = std::make_unique<Scene>(vulkanContext_, dataArray, supportDeviceAddress);

	constexpr uint32_t xCount = 2;
	constexpr uint32_t zCount = 4;
	constexpr float dist = 1.2f;
	constexpr float xMidPos = static_cast<float>(xCount - 1) * dist / 2.0f;
	constexpr float zMidPos = static_cast<float>(zCount - 1) * dist / 2.0f;
	constexpr float yPos = -0.33f;
	const glm::vec3 scale = glm::vec3(0.5f, 0.5f, 0.5f);

	for (int x = 0; x < xCount; ++x)
	{
		for (int z = 0; z < zCount; ++z)
		{
			float xPos = x * dist - xMidPos;
			float zPos = -(z * dist) + zMidPos;
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

void AppSkinning::UpdateUI()
{
	if (!ShowImGui())
	{
		imguiPtr_->ImGuiDrawEmpty();
		return;
	}

	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("Compute-based Skinning", 500, 650);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);

	ImGui::Text("Triangle Count: %i", scene_->triangleCount_);
	ImGui::Checkbox("Render Lights", &inputContext_.renderLights_);
	ImGui::SeparatorText("Shading");
	imguiPtr_->ImGuiShowPBRConfig(&inputContext_.pbrPC_, resourcesIBL_->cubemapMipmapCount_);

	ImGui::SeparatorText("Shadow mapping");
	ImGui::SliderFloat("Min Bias", &inputContext_.shadowMinBias_, 0.f, 0.01f);
	ImGui::SliderFloat("Max Bias", &inputContext_.shadowMaxBias_, 0.f, 0.01f);
	ImGui::SliderFloat("Near Plane", &inputContext_.shadowNearPlane_, 0.1f, 50.0f);
	ImGui::SliderFloat("Far Plane", &inputContext_.shadowFarPlane_, 10.0f, 150.0f);
	ImGui::SliderFloat("Ortho Size", &inputContext_.shadowOrthoSize_, 10.0f, 100.0f);

	ImGui::SeparatorText("Light position");
	ImGui::SliderFloat("X", &(inputContext_.shadowCasterPosition_[0]), -10.0f, 10.0f);
	ImGui::SliderFloat("Y", &(inputContext_.shadowCasterPosition_[1]), 5.0f, 60.0f);
	ImGui::SliderFloat("Z", &(inputContext_.shadowCasterPosition_[2]), -10.0f, 10.0f);

	imguiPtr_->ImGuiEnd();

	for (auto& pipeline : pipelines_)
	{
		pipeline->UpdateFromInputContext(vulkanContext_, inputContext_);
	}
	for (auto& resources : resources_)
	{
		resources->UpdateFromInputContext(vulkanContext_, inputContext_);
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