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
	resShadow_ = std::make_unique<ResourcesShadow>();
	resShadow_->CreateSingleShadowMap(vulkanContext_);

	InitLights();

	// Initialize attachments
	InitSharedResources();

	// Image-Based Lighting
	resIBL_ = std::make_unique<ResourcesIBL>(vulkanContext_, AppConfig::TextureFolder + "piazza_bologni_1k.hdr");
	cubemapMipmapCount_ = static_cast<float>(Utility::MipMapCount(IBLConfig::InputCubeSideLength));

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
	clearPtr_ = std::make_unique<PipelineClear>(vulkanContext_); // This is responsible to clear swapchain image
	skyboxPtr_ = std::make_unique<PipelineSkybox>(
		vulkanContext_,
		&(resIBL_->diffuseCubemap_),
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
	imguiPtr_ = std::make_unique<PipelineImGui>(vulkanContext_, vulkanInstance_.GetInstance(), glfwWindow_);
	// Present swapchain image
	finishPtr_ = std::make_unique<PipelineFinish>(vulkanContext_);

	// Put all renderer pointers to a vector
	pipelines_ =
	{
		// Must be in order
		clearPtr_.get(),
		skyboxPtr_.get(),
		shadowPtr_.get(),
		pbrPtr_.get(),
		lightPtr_.get(), // Should be the last
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
		{.color_ = glm::vec4(1.f), .radius_ = 1.0f },

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
	skyboxUbo.view = glm::mat4(glm::mat3(skyboxUbo.view)); // Remove translation
	skyboxPtr_->SetCameraUBO(vulkanContext_, skyboxUbo);

	shadowPtr_->UpdateShadow(vulkanContext_, resShadow_.get(), resLight_->lights_[0].position_);

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
	static float staticLightPos[3] = { -2.5f, 20.0f, 5.0f };
	static float staticNearPlane = 0.1f;
	static float staticFarPlane = staticLightPos[1] + 10;
	static float staticOrthoSize = 10.0f;
	static float staticMinBias = 0.001f;
	static float staticMaxBias = 0.001f;

	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("Shadow Mapping", 525, 650);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);

	ImGui::Text("Vertices: %i, Indices: %i", scene_->vertices_.size(), scene_->indices_.size());
	ImGui::Checkbox("Render Lights", &staticLightRender);
	ImGui::SeparatorText("Shading");
	imguiPtr_->ImGuiShowPBRConfig(&staticPBRPushConstants, cubemapMipmapCount_);

	ImGui::SeparatorText("Shadow mapping");
	ImGui::SliderFloat("Min Bias", &staticMinBias, 0.f, 0.01f);
	ImGui::SliderFloat("Max Bias", &staticMaxBias, 0.f, 0.01f);
	ImGui::SliderFloat("Near Plane", &staticNearPlane, 0.1f, 50.0f);
	ImGui::SliderFloat("Far Plane", &staticFarPlane, 10.0f, 150.0f);
	ImGui::SliderFloat("Ortho Size", &staticOrthoSize, 10.0f, 30.0f);

	ImGui::SeparatorText("Light position");
	ImGui::SliderFloat("X", &(staticLightPos[0]), -10.0f, 10.0f);
	ImGui::SliderFloat("Y", &(staticLightPos[1]), 15.0f, 60.0f);
	ImGui::SliderFloat("Z", &(staticLightPos[2]), -10.0f, 10.0f);

	imguiPtr_->ImGuiEnd();

	// TODO Check if light position is changed 
	resLight_->UpdateLightPosition(vulkanContext_, 0, &(staticLightPos[0]));

	lightPtr_->ShouldRender(staticLightRender);
	pbrPtr_->SetPBRPushConstants(staticPBRPushConstants);

	resShadow_->shadowUBO_.shadowMinBias = staticMinBias;
	resShadow_->shadowUBO_.shadowMaxBias = staticMaxBias;
	resShadow_->shadowNearPlane_ = staticNearPlane;
	resShadow_->shadowFarPlane_ = staticFarPlane;
	resShadow_->orthoSize_ = staticOrthoSize;
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

	DestroyResources();
	DestroyInternal();
}