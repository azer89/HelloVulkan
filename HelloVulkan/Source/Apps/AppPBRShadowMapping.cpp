#include "AppPBRShadowMapping.h"
#include "Configs.h"
#include "VulkanUtility.h"
#include "PipelineEquirect2Cube.h"
#include "PipelineCubeFilter.h"
#include "PipelineBRDFLUT.h"

#include <glm/gtc/matrix_transform.hpp>

#include "imgui_impl_glfw.h"
#include "imgui_impl_volk.h"

AppPBRShadowMapping::AppPBRShadowMapping()
{
}

void AppPBRShadowMapping::Init()
{
	// Init shadow map
	uint32_t shadowMapSize = 2048;
	shadowMap_.CreateDepthResources(
		vulkanContext_,
		shadowMapSize,
		shadowMapSize,
		VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_USAGE_SAMPLED_BIT);
	shadowMap_.CreateDefaultSampler(vulkanContext_);
	shadowMap_.SetDebugName(vulkanContext_, "Shadow_Map_Image");

	// Initialize lights
	InitLights();

	// Initialize attachments
	InitSharedImageResources();

	// Image-Based Lighting
	InitIBLResources(AppConfig::TextureFolder + "piazza_bologni_1k.hdr");
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
	clearPtr_ = std::make_unique<PipelineClear>(
		vulkanContext_);
	// This draws a cube
	skyboxPtr_ = std::make_unique<PipelineSkybox>(
		vulkanContext_,
		&(iblResources_->environmentCubemap_),
		&depthImage_,
		&multiSampledColorImage_,
		// This is the first offscreen render pass so
		// we need to clear the color attachment and depth attachment
		RenderPassBit::ColorClear | 
		RenderPassBit::DepthClear
	);
	pbrPtr_ = std::make_unique<PipelinePBRShadowMapping>(
		vulkanContext_,
		scene_.get(),
		&lights_,
		iblResources_.get(),
		&shadowMap_,
		&depthImage_,
		&multiSampledColorImage_);
	shadowPtr_ = std::make_unique<PipelineShadow>(vulkanContext_, scene_.get(), &shadowMap_);
	lightPtr_ = std::make_unique<PipelineLightRender>(
		vulkanContext_,
		&lights_,
		&depthImage_,
		&multiSampledColorImage_
	);
	// Resolve multiSampledColorImage_ to singleSampledColorImage_
	resolveMSPtr_ = std::make_unique<PipelineResolveMS>(
		vulkanContext_, &multiSampledColorImage_, &singleSampledColorImage_);
	// This is on-screen render pass that transfers 
	// singleSampledColorImage_ to swapchain image
	tonemapPtr_ = std::make_unique<PipelineTonemap>(
		vulkanContext_,
		&singleSampledColorImage_
	);
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

void AppPBRShadowMapping::InitLights()
{
	// Lights (SSBO)
	lights_.AddLights(vulkanContext_,
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

void AppPBRShadowMapping::DestroyResources()
{
	shadowMap_.Destroy();

	// IBL Images
	iblResources_.reset();

	// Destroy meshes
	scene_.reset();

	// Lights
	lights_.Destroy();

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

void AppPBRShadowMapping::UpdateUBOs()
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
	LightData light = lights_.lights_[0];
	glm::mat4 lightProjection = glm::perspective(glm::radians(45.f), 1.0f, shadowUBO_.shadowNearPlane, shadowUBO_.shadowFarPlane);
	glm::mat4 lightView = glm::lookAt(glm::vec3(light.position_), glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
	glm::mat4 lightSpaceMatrix = lightProjection * lightView;
	shadowUBO_.lightSpaceMatrix = lightSpaceMatrix;
	shadowUBO_.lightPosition = light.position_;
	shadowPtr_->SetShadowMapUBO(vulkanContext_, shadowUBO_);
	pbrPtr_->SetShadowMapConfigUBO(vulkanContext_, shadowUBO_);
}

void AppPBRShadowMapping::UpdateUI()
{
	if (!showImgui_)
	{
		imguiPtr_->DrawEmptyImGui();
		return;
	}

	static bool staticLightRender = true;
	static PushConstantPBR staticPBRPushConstants =
	{
		.lightIntensity = 1.5f,
		.baseReflectivity = 0.01f,
		.maxReflectionLod = 1.f,
		.lightFalloff = 0.1f,
		.albedoMultipler = 0.5
	};
	static ShadowMapUBO staticShadowUBO =
	{
		.shadowMinBias = 0.001f,
		.shadowMaxBias = 0.001f,
		.shadowNearPlane = 10.0f,
		.shadowFarPlane = 40.0f,
		.pcfScale = 0.5
	};
	static float staticLightPos[3] = { -5.f, 30.0f, 5.0f};
	static int staticPCFIteration = 2;

	imguiPtr_->StartImGui();

	ImGui::SetNextWindowSize(ImVec2(525, 560));
	ImGui::Begin("Bindless Shadow Mapping");
	ImGui::SetWindowFontScale(1.25f);
	
	ImGui::Text("FPS: %.0f", (1.f / deltaTime_));
	ImGui::Text("Vertices: %i, Indices: %i", scene_->vertices_.size(), scene_->indices_.size());

	ImGui::SeparatorText("Shading");
	ImGui::Checkbox("Render Lights", &staticLightRender);
	ImGui::SliderFloat("Light Falloff", &staticPBRPushConstants.lightFalloff, 0.01f, 5.f);
	ImGui::SliderFloat("Light Intensity", &staticPBRPushConstants.lightIntensity, 0.1f, 100.f);
	ImGui::SliderFloat("Albedo Multiplier", &staticPBRPushConstants.albedoMultipler, 0.0f, 1.0f);
	ImGui::SliderFloat("Base Reflectivity", &staticPBRPushConstants.baseReflectivity, 0.01f, 1.f);
	ImGui::SliderFloat("Max Mipmap Lod", &staticPBRPushConstants.maxReflectionLod, 0.1f, cubemapMipmapCount_);

	ImGui::SeparatorText("Shadow mapping");
	ImGui::SliderFloat("Min Bias", &staticShadowUBO.shadowMinBias, 0.00001f, 0.01f);
	ImGui::SliderFloat("Max Bias", &staticShadowUBO.shadowMaxBias, 0.001f, 0.1f);
	ImGui::SliderFloat("Near Plane", &staticShadowUBO.shadowNearPlane, 0.1f, 50.0f);
	ImGui::SliderFloat("Far Plane", &staticShadowUBO.shadowFarPlane, 10.0f, 150.0f);
	ImGui::SliderFloat("PCF Scale", &staticShadowUBO.pcfScale, 0.1, 1.0);
	ImGui::SliderInt("PCF Iteration", &staticPCFIteration, 1, 10);

	ImGui::SeparatorText("Light position");
	ImGui::SliderFloat("X", &(staticLightPos[0]), -10.0f, 10.0f);
	ImGui::SliderFloat("Y", &(staticLightPos[1]), 20.0f, 60.0f);
	ImGui::SliderFloat("Z", &(staticLightPos[2]), -10.0f, 10.0f);

	imguiPtr_->EndImGui();

	lights_.UpdateLightPosition(vulkanContext_, 0, &(staticLightPos[0]));

	lightPtr_->RenderEnable(staticLightRender);
	pbrPtr_->SetPBRPushConstants(staticPBRPushConstants);

	shadowUBO_.shadowMinBias = staticShadowUBO.shadowMinBias;
	shadowUBO_.shadowMaxBias = staticShadowUBO.shadowMaxBias;
	shadowUBO_.shadowNearPlane = staticShadowUBO.shadowNearPlane;
	shadowUBO_.shadowFarPlane = staticShadowUBO.shadowFarPlane;
	shadowUBO_.pcfScale = staticShadowUBO.pcfScale;
	shadowUBO_.pcfIteration = staticPCFIteration;
}

// This is called from main.cpp
int AppPBRShadowMapping::MainLoop()
{
	InitVulkan({
		.supportRaytracing_ = false,
		.supportMSAA_ = true
	});
	Init();

	// Main loop
	while (!GLFWWindowShouldClose())
	{
		PollEvents();
		ProcessTiming();
		ProcessInput();
		DrawFrame();
	}

	// Wait until everything is finished
	vkDeviceWaitIdle(vulkanContext_.GetDevice());

	DestroyResources();
	Terminate();

	return 0;
}