#include "AppPBRClusterForward.h"
#include "PipelineEquirect2Cube.h"
#include "PipelineCubeFilter.h"
#include "PipelineBRDFLUT.h"
#include "Configs.h"
#include "PushConstants.h"
#include "VulkanUtility.h"

#include "glm/gtc/matrix_transform.hpp"

#include "imgui_impl_glfw.h"
#include "imgui_impl_volk.h"

AppPBRClusterForward::AppPBRClusterForward() 
{
}

void AppPBRClusterForward::Init()
{
	// Initialize attachments
	InitSharedImageResources();

	// Initialize lights
	InitLights();

	// Image-Based Lighting
	InitIBLResources(AppConfig::TextureFolder + "dikhololo_night_4k.hdr");
	cubemapMipmapCount_ = static_cast<float>(Utility::MipMapCount(IBLConfig::InputCubeSideLength));

	cfBuffers_ = std::make_unique<ClusterForwardBuffers>();
	cfBuffers_->CreateBuffers(vulkanContext_, lights_.GetLightCount());

	// glTF model
	model_ = std::make_unique<Model>();
	model_->LoadSlotBased(vulkanContext_, 
		AppConfig::ModelFolder + "Sponza//Sponza.gltf");
	std::vector<Model*> models = { model_.get()};

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
	aabbPtr_ = std::make_unique<PipelineAABBGenerator>(vulkanContext_, cfBuffers_.get());
	lightCullPtr_ = std::make_unique<PipelineLightCulling>(vulkanContext_, &lights_, cfBuffers_.get());
	pbrPtr_ = std::make_unique<PipelinePBRClusterForward>(
		vulkanContext_,
		models,
		&lights_,
		cfBuffers_.get(),
		iblResources_.get(),
		&depthImage_,
		&multiSampledColorImage_);
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
		skyboxPtr_.get(),
		aabbPtr_.get(),
		lightCullPtr_.get(),
		pbrPtr_.get(),
		lightPtr_.get(),
		resolveMSPtr_.get(),
		tonemapPtr_.get(),
		imguiPtr_.get(),
		finishPtr_.get()
	};
}

void AppPBRClusterForward::InitLights()
{
	std::vector<LightData> lights;

	float pi2 = glm::two_pi<float>();
	constexpr uint32_t NR_LIGHTS = 1000;
	for (uint32_t i = 0; i < NR_LIGHTS; ++i)
	{
		float yPos = Utility::RandomNumber(-2.f, 10.0f);
		float radius = Utility::RandomNumber(0.0f, 10.0f);
		float rad = Utility::RandomNumber(0.0f, pi2);
		float xPos = glm::cos(rad);

		glm::vec4 position(
			glm::cos(rad) * radius,
			yPos,
			glm::sin(rad) * radius,
			1.f
		);

		glm::vec4 color(
			Utility::RandomNumber(0.0f, 1.0f),
			Utility::RandomNumber(0.0f, 1.0f),
			Utility::RandomNumber(0.0f, 1.0f),
			1.f
		);

		LightData l;
		l.color_ = color;
		l.position_ = position;
		l.radius_ = Utility::RandomNumber(0.5f, 2.0f);

		lights.push_back(l);
	}

	lights_.AddLights(vulkanContext_, lights);
}

void AppPBRClusterForward::DestroyResources()
{
	// IBL Images
	iblResources_.reset();

	// Destroy meshes
	model_->Destroy();
	model_.reset();

	// Lights
	lights_.Destroy();

	cfBuffers_.reset();

	// Destroy renderers
	clearPtr_.reset();
	finishPtr_.reset();
	skyboxPtr_.reset();
	aabbPtr_.reset();
	lightCullPtr_.reset();
	pbrPtr_.reset();
	lightPtr_.reset();
	resolveMSPtr_.reset();
	tonemapPtr_.reset();
	imguiPtr_.reset();
}

void AppPBRClusterForward::UpdateUBOs()
{
	// Camera UBO
	CameraUBO ubo = camera_->GetCameraUBO();
	lightPtr_->SetCameraUBO(vulkanContext_, ubo);
	pbrPtr_->SetCameraUBO(vulkanContext_, ubo);

	// Remove translation for skybox
	CameraUBO skyboxUbo = ubo;
	skyboxUbo.view = glm::mat4(glm::mat3(skyboxUbo.view));
	skyboxPtr_->SetCameraUBO(vulkanContext_, skyboxUbo);

	// Model UBOs
	ModelUBO modelUBO1
	{
		.model = glm::mat4(1.0)
	};
	model_->SetModelUBO(vulkanContext_, modelUBO1);

	// Clustered forward
	ClusterForwardUBO cfUBO = camera_->GetClusterForwardUBO();
	aabbPtr_->SetClusterForwardUBO(vulkanContext_, cfUBO);
	lightCullPtr_->ResetGlobalIndex(vulkanContext_);
	lightCullPtr_->SetClusterForwardUBO(vulkanContext_, cfUBO);
	pbrPtr_->SetClusterForwardUBO(vulkanContext_, cfUBO);
}

void AppPBRClusterForward::UpdateUI()
{
	if (!showImgui_)
	{
		imguiPtr_->DrawEmptyImGui();
		return;
	}

	static bool lightRender = true;
	static PushConstantPBR pbrPC;

	imguiPtr_->StartImGui();

	ImGui::SetNextWindowSize(ImVec2(525, 250));
	ImGui::Begin(AppConfig::ScreenTitle.c_str());
	ImGui::SetWindowFontScale(1.25f);
	ImGui::Text("FPS : %.0f", (1.f / deltaTime_));
	ImGui::Checkbox("Render Lights", &lightRender);
	ImGui::SliderFloat("Light Falloff", &pbrPC.lightFalloff, 0.01f, 5.f);
	ImGui::SliderFloat("Light Intensity", &pbrPC.lightIntensity, 0.1f, 100.f);
	ImGui::SliderFloat("Albedo Multiplier", &pbrPC.albedoMultipler, 0.0f, 1.0f);
	ImGui::SliderFloat("Base Reflectivity", &pbrPC.baseReflectivity, 0.01f, 1.f);
	ImGui::SliderFloat("Max Mipmap Lod", &pbrPC.maxReflectionLod, 0.1f, cubemapMipmapCount_);

	imguiPtr_->EndImGui();

	lightPtr_->RenderEnable(lightRender);
	pbrPtr_->SetPBRPushConstants(pbrPC);
}

// This is called from main.cpp
int AppPBRClusterForward::MainLoop()
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