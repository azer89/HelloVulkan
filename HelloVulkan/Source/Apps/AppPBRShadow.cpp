#include "AppPBRShadow.h"
#include "Configs.h"
#include "VulkanUtility.h"
#include "PipelineEquirect2Cube.h"
#include "PipelineCubeFilter.h"
#include "PipelineBRDFLUT.h"

#include <glm/gtc/matrix_transform.hpp>

#include "imgui_impl_glfw.h"
#include "imgui_impl_volk.h"

AppPBRShadow::AppPBRShadow()
{
}

void AppPBRShadow::Init()
{
	// Init shadow map
	resShadow_ = std::make_unique<ResourcesShadow>();
	resShadow_->CreateCascadeShadowMap(vulkanContext_);

	// Initialize lights
	InitLights();

	// Initialize attachments
	InitSharedResources();

	// Image-Based Lighting
	resIBL_ = std::make_unique<ResourcesIBL>(vulkanContext_, AppConfig::TextureFolder + "piazza_bologni_1k.hdr");
	cubemapMipmapCount_ = static_cast<float>(Utility::MipMapCount(IBLConfig::InputCubeSideLength));

	std::vector<std::string> modelFiles = {
		AppConfig::ModelFolder + "Sponza//Sponza.gltf",
		AppConfig::ModelFolder + "Tachikoma//Tachikoma.gltf",
		AppConfig::ModelFolder + "Hexapod//Hexapod.gltf"
	};
	scene_ = std::make_unique<Scene>(vulkanContext_, modelFiles);

	// Model matrix for Tachikoma
	glm::mat4 modelMatrix(1.f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.15f, 0.35f, 1.5f));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(45.f), glm::vec3(0.f, 1.f, 0.f));
	modelMatrix = glm::scale(modelMatrix, glm::vec3(0.7f, 0.7f, 0.7f));
	scene_->UpdateModelMatrix(vulkanContext_, { .model = modelMatrix }, 1);

	// Model matrix for Hexapod
	modelMatrix = glm::mat4(1.f);
	modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.62f, -1.5f));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f));
	scene_->UpdateModelMatrix(vulkanContext_, { .model = modelMatrix }, 2);

	// Pipelines
	// This is responsible to clear swapchain image
	clearPtr_ = std::make_unique<PipelineClear>(vulkanContext_);
	// This draws a cube
	skyboxPtr_ = std::make_unique<PipelineSkybox>(
		vulkanContext_,
		&(resIBL_->environmentCubemap_),
		resShared_.get(),
		// This is the first offscreen render pass so we need to clear the color attachment and depth attachment
		RenderPassBit::ColorClear | RenderPassBit::DepthClear);
	pbrPtr_ = std::make_unique<PipelinePBRShadow>(
		vulkanContext_,
		scene_.get(),
		resLight_.get(),
		resIBL_.get(),
		resShadow_.get(),
		resShared_.get());
	shadowPtr_ = std::make_unique<PipelineShadow>(vulkanContext_, scene_.get(), resShadow_.get());
	lightPtr_ = std::make_unique<PipelineLightRender>(vulkanContext_, resLight_.get(), resShared_.get());
	// Resolve multiSampledColorImage_ to singleSampledColorImage_
	resolveMSPtr_ = std::make_unique<PipelineResolveMS>(vulkanContext_, resShared_.get());
	// This is on-screen render pass that transfers singleSampledColorImage_ to swapchain image
	tonemapPtr_ = std::make_unique<PipelineTonemap>(vulkanContext_, &(resShared_->singleSampledColorImage_));
	// ImGui here
	imguiPtr_ = std::make_unique<PipelineImGui>(vulkanContext_, vulkanInstance_.GetInstance(), glfwWindow_);
	// Present swapchain image
	finishPtr_ = std::make_unique<PipelineFinish>(vulkanContext_);

	// Put all renderer pointers to a vector
	pipelines_ =
	{
		// Must be in order
		clearPtr_.get(),
		shadowPtr_.get(),
		skyboxPtr_.get(),
		pbrPtr_.get(),
		lightPtr_.get(),
		resolveMSPtr_.get(),
		tonemapPtr_.get(),
		imguiPtr_.get(),
		finishPtr_.get()
	};
}

void AppPBRShadow::InitLights()
{
	// Lights (SSBO)
	resLight_ = std::make_unique<ResourcesLight>();
	resLight_->AddLights(vulkanContext_,
	{
		// The first light is used to generate the shadow map
		// and its position is set by ImGui
		{ .color_ = glm::vec4(1.f), .radius_ = 1.0f },

		// Add additional lights so that the scene is not too dark
		{.position_ = glm::vec4(-1.5f, 2.5f,  5.f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{.position_ = glm::vec4(1.5f, 2.5f,  5.f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{.position_ = glm::vec4(-1.5f, 2.5f, -5.f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{.position_ = glm::vec4(1.5f, 2.5f, -5.f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f }
	});
}

void AppPBRShadow::DestroyResources()
{
	// Resources
	resShadow_.reset();
	resIBL_.reset();
	resLight_.reset();

	// Destroy meshes
	scene_.reset();
	
	// Destroy renderers
	clearPtr_.reset();
	finishPtr_.reset();
	skyboxPtr_.reset();
	pbrPtr_.reset();
	shadowPtr_.reset();
	lightPtr_.reset();
	resolveMSPtr_.reset();
	tonemapPtr_.reset();
	imguiPtr_.reset();
}

void AppPBRShadow::UpdateUBOs()
{
	// Camera matrices
	CameraUBO ubo = camera_->GetCameraUBO();
	lightPtr_->SetCameraUBO(vulkanContext_, ubo);
	pbrPtr_->SetCameraUBO(vulkanContext_, ubo);

	// Skybox
	CameraUBO skyboxUbo = ubo;
	skyboxUbo.view = glm::mat4(glm::mat3(skyboxUbo.view));
	skyboxPtr_->SetCameraUBO(vulkanContext_, skyboxUbo);

	shadowPtr_->CalculateCascade(vulkanContext_, camera_.get(), & (resLight_->lights_[0]), &shadowUBO_);
	pbrPtr_->SetShadowMapConfigUBO(vulkanContext_, shadowUBO_);
}

void AppPBRShadow::UpdateUI()
{
	if (!showImgui_)
	{
		imguiPtr_->ImGuiDrawEmpty();
		return;
	}

	static bool staticLightRender = true;
	static PushConstPBR staticPBRPushConstants =
	{
		.lightIntensity = 1.5f,
		.baseReflectivity = 0.01f,
		.maxReflectionLod = 1.f,
		.lightFalloff = 0.1f,
		.albedoMultipler = 0.5
	};
	static ShadowMapUBO staticShadowUBO =
	{
		.shadowMinBias = 0.002f,
		.shadowMaxBias = 0.002f,
		.cameraNearPlane = CameraConfig::Near,
		.cameraFarPlane = CameraConfig::Far,
		.poissonSize = 5.f
	};
	static float staticLightPos[3] = { -5.f, 30.0f, 5.0f};

	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("Cascade Shadow Mapping", 525, 550);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);

	ImGui::Text("Vertices: %i, Indices: %i", scene_->vertices_.size(), scene_->indices_.size());
	ImGui::SeparatorText("Shading");
	ImGui::Checkbox("Render Lights", &staticLightRender);
	imguiPtr_->ImGuiShowPBRConfig(&staticPBRPushConstants, cubemapMipmapCount_);

	ImGui::SeparatorText("Shadow mapping");
	ImGui::SliderFloat("Min Bias", &staticShadowUBO.shadowMinBias, 0.00001f, 0.01f);
	ImGui::SliderFloat("Max Bias", &staticShadowUBO.shadowMaxBias, 0.001f, 0.1f);
	ImGui::SliderFloat("Poisson Kernel", &staticShadowUBO.poissonSize, 1.f, 10.f);

	ImGui::SeparatorText("Light position");
	ImGui::SliderFloat("X", &(staticLightPos[0]), -10.0f, 10.0f);
	ImGui::SliderFloat("Y", &(staticLightPos[1]), 20.0f, 60.0f);
	ImGui::SliderFloat("Z", &(staticLightPos[2]), -10.0f, 10.0f);

	imguiPtr_->ImGuiEnd();

	// TODO Check if light position is changed 
	resLight_->UpdateLightPosition(vulkanContext_, 0, &(staticLightPos[0]));

	lightPtr_->RenderEnable(staticLightRender);
	pbrPtr_->SetPBRPushConstants(staticPBRPushConstants);

	shadowUBO_.shadowMinBias = staticShadowUBO.shadowMinBias;
	shadowUBO_.shadowMaxBias = staticShadowUBO.shadowMaxBias;
	shadowUBO_.poissonSize = staticShadowUBO.poissonSize;
}

// This is called from main.cpp
void AppPBRShadow::MainLoop()
{
	InitVulkan({
		.supportRaytracing_ = false,
		.supportMSAA_ = true,
		.supportAnisotropicFilterting = true
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