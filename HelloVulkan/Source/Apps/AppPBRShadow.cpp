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
	resShadow_->CreateSingleShadowMap(vulkanContext_);

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
		resLights_.get(),
		resIBL_.get(),
		resShadow_.get(),
		resShared_.get());
	shadowPtr_ = std::make_unique<PipelineShadow>(vulkanContext_, scene_.get(), resShadow_.get());
	lightPtr_ = std::make_unique<PipelineLightRender>(vulkanContext_, resLights_.get(), resShared_.get());
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
	resLights_ = std::make_unique<ResourcesLight>();
	resLights_->AddLights(vulkanContext_,
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
	resLights_.reset();

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

	// Shadow mapping
	LightData light = resLights_->lights_[0];
	//glm::mat4 lightProjection = glm::perspective(glm::radians(45.f), 1.0f, shadowUBO_.shadowNearPlane, shadowUBO_.shadowFarPlane);
	glm::mat4 lightProjection = glm::ortho(
		-10.f, 
		10.f, 
		-10.f, 
		10.f, 
		resShadow_->shadowNearPlane_, 
		resShadow_->shadowFarPlane_);
	glm::mat4 lightView = glm::lookAt(glm::vec3(light.position_), glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
	glm::mat4 lightSpaceMatrix = lightProjection * lightView;
	resShadow_->shadowUBO_.lightSpaceMatrix = lightSpaceMatrix;
	resShadow_->shadowUBO_.lightPosition = light.position_;
	shadowPtr_->SetShadowMapUBO(vulkanContext_, resShadow_->shadowUBO_);
	pbrPtr_->SetShadowMapConfigUBO(vulkanContext_, resShadow_->shadowUBO_);
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
	static float staticLightPos[3] = { -5.f, 20.0f, 5.0f };
	static float staticNearPlane = 0.1f;
	static float staticFarPlane = staticLightPos[1] + 10;
	static ShadowMapUBO staticShadowUBO =
	{
		.shadowMinBias = 0.001f,
		.shadowMaxBias = 0.001f
	};

	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("Shadow Mapping", 525, 650);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);

	ImGui::Text("Vertices: %i, Indices: %i", scene_->vertices_.size(), scene_->indices_.size());
	ImGui::SeparatorText("Shading");
	ImGui::Checkbox("Render Lights", &staticLightRender);
	imguiPtr_->ImGuiShowPBRConfig(&staticPBRPushConstants, cubemapMipmapCount_);

	ImGui::SeparatorText("Shadow mapping");
	ImGui::SliderFloat("Min Bias", &staticShadowUBO.shadowMinBias, 0.f, 0.01f);
	ImGui::SliderFloat("Max Bias", &staticShadowUBO.shadowMaxBias, 0.f, 0.01f);
	ImGui::SliderFloat("Near Plane", &staticNearPlane, 0.1f, 50.0f);
	ImGui::SliderFloat("Far Plane", &staticFarPlane, 10.0f, 150.0f);

	ImGui::SeparatorText("Light position");
	ImGui::SliderFloat("X", &(staticLightPos[0]), -10.0f, 10.0f);
	ImGui::SliderFloat("Y", &(staticLightPos[1]), 15.0f, 60.0f);
	ImGui::SliderFloat("Z", &(staticLightPos[2]), -10.0f, 10.0f);

	imguiPtr_->ImGuiEnd();

	// TODO Check if light position is changed 
	resLights_->UpdateLightPosition(vulkanContext_, 0, &(staticLightPos[0]));

	lightPtr_->RenderEnable(staticLightRender);
	pbrPtr_->SetPBRPushConstants(staticPBRPushConstants);

	resShadow_->shadowUBO_.shadowMinBias = staticShadowUBO.shadowMinBias;
	resShadow_->shadowUBO_.shadowMaxBias = staticShadowUBO.shadowMaxBias;
	resShadow_->shadowNearPlane_ = staticNearPlane;
	resShadow_->shadowFarPlane_ = staticFarPlane;
}

// This is called from main.cpp
void AppPBRShadow::MainLoop()
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