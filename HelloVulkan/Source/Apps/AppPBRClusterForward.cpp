#include "AppPBRClusterForward.h"
#include "PushConstants.h"
#include "VulkanUtility.h"
#include "Configs.h"

#include "glm/ext.hpp"
#include "imgui_impl_vulkan.h"

AppPBRClusterForward::AppPBRClusterForward() 
{
}

void AppPBRClusterForward::Init()
{
	camera_->SetPositionAndTarget(glm::vec3(0.0f, 1.0f, 6.0f), glm::vec3(0.0, 2.5, 0.0));

	// Initialize attachments
	InitSharedResources();

	// Initialize lights
	InitLights();

	// Image-Based Lighting
	resIBL_ = std::make_unique<ResourcesIBL>(vulkanContext_, AppConfig::TextureFolder + "dikhololo_night_4k.hdr");

	resCF_ = std::make_unique<ResourcesClusterForward>();
	resCF_->CreateBuffers(vulkanContext_, resLight_->GetLightCount());

	std::vector<ModelCreateInfo> dataArray = {
		{AppConfig::ModelFolder + "Sponza/Sponza.gltf", 1}
	};
	bool supportDeviceAddress = true;
	scene_ = std::make_unique<Scene>(vulkanContext_, dataArray, supportDeviceAddress);

	// Pipelines
	clearPtr_ = std::make_unique<PipelineClear>(vulkanContext_); // This is responsible to clear swapchain image
	skyboxPtr_ = std::make_unique<PipelineSkybox>(
		vulkanContext_,
		&(resIBL_->environmentCubemap_),
		resShared_.get(),
		// This is the first offscreen render pass so we need to clear the color attachment and depth attachment
		RenderPassBit::ColorClear | RenderPassBit::DepthClear);
	aabbPtr_ = std::make_unique<PipelineAABBGenerator>(vulkanContext_, resCF_.get());
	lightCullPtr_ = std::make_unique<PipelineLightCulling>(vulkanContext_, resLight_.get(), resCF_.get());
	pbrOpaquePtr_ = std::make_unique<PipelinePBRClusterForward>(
		vulkanContext_,
		scene_.get(),
		resLight_.get(),
		resCF_.get(),
		resIBL_.get(),
		resShared_.get(),
		MaterialType::Opaque);
	pbrTransparentPtr_ = std::make_unique<PipelinePBRClusterForward>(
		vulkanContext_,
		scene_.get(),
		resLight_.get(),
		resCF_.get(),
		resIBL_.get(),
		resShared_.get(),
		MaterialType::Transparent);
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
		aabbPtr_.get(),
		lightCullPtr_.get(),
		pbrOpaquePtr_.get(),
		pbrTransparentPtr_.get(),
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

	resLight_ = std::make_unique<ResourcesLight>();
	resLight_->AddLights(vulkanContext_, lights);
}

void AppPBRClusterForward::DestroyResources()
{
	// Resources
	resIBL_.reset();
	resCF_.reset();
	resLight_.reset();

	// Destroy all objects
	scene_.reset();

	// Destroy renderers
	clearPtr_.reset();
	finishPtr_.reset();
	skyboxPtr_.reset();
	aabbPtr_.reset();
	lightCullPtr_.reset();
	pbrOpaquePtr_.reset();
	pbrTransparentPtr_.reset();
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
	pbrOpaquePtr_->SetCameraUBO(vulkanContext_, ubo);
	pbrTransparentPtr_->SetCameraUBO(vulkanContext_, ubo);

	// Remove translation for skybox
	CameraUBO skyboxUbo = ubo;
	skyboxUbo.view = glm::mat4(glm::mat3(skyboxUbo.view));
	skyboxPtr_->SetCameraUBO(vulkanContext_, skyboxUbo);

	// Clustered forward
	ClusterForwardUBO cfUBO = camera_->GetClusterForwardUBO();
	aabbPtr_->SetClusterForwardUBO(vulkanContext_, cfUBO);
	lightCullPtr_->ResetGlobalIndex(vulkanContext_);
	lightCullPtr_->SetClusterForwardUBO(vulkanContext_, cfUBO);
	pbrOpaquePtr_->SetClusterForwardUBO(vulkanContext_, cfUBO);
	pbrTransparentPtr_->SetClusterForwardUBO(vulkanContext_, cfUBO);
}

void AppPBRClusterForward::UpdateUI()
{
	if (!showImgui_)
	{
		imguiPtr_->ImGuiDrawEmpty();
		return;
	}

	static bool lightRender = true;
	static PushConstPBR pbrPC;

	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("Clustered Forward Shading", 500, 325);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);
	ImGui::Checkbox("Render Lights", &lightRender);
	imguiPtr_->ImGuiShowPBRConfig(&pbrPC, resIBL_->cubemapMipmapCount_);
	imguiPtr_->ImGuiEnd();

	lightPtr_->ShouldRender(lightRender);
	pbrOpaquePtr_->SetPBRPushConstants(pbrPC);
	pbrTransparentPtr_->SetPBRPushConstants(pbrPC);
}

// This is called from main.cpp
void AppPBRClusterForward::MainLoop()
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