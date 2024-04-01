#include "AppPBRClusterForward.h"
#include "PushConstants.h"
#include "Utility.h"
#include "Configs.h"

#include "PipelineSkybox.h"
#include "PipelineClear.h"
#include "PipelineFinish.h"
#include "PipelineTonemap.h"
#include "PipelineResolveMS.h"

#include "glm/ext.hpp"
#include "imgui_impl_vulkan.h"

AppPBRClusterForward::AppPBRClusterForward() 
{
}

void AppPBRClusterForward::Init()
{
	uiData_.pbrPC_.albedoMultipler = 0.01f;
	camera_->SetPositionAndTarget(glm::vec3(0.0f, 1.0f, 6.0f), glm::vec3(0.0, 2.5, 0.0));

	// Initialize attachments
	InitSharedResources();

	// Initialize lights
	InitLights();

	// Image-Based Lighting
	resourcesIBL_ = AddResources<ResourcesIBL>(vulkanContext_, AppConfig::TextureFolder + "dikhololo_night_4k.hdr");

	resCF_ = AddResources<ResourcesClusterForward>();
	resCF_->CreateBuffers(vulkanContext_, resourcesLight_->GetLightCount());

	std::vector<ModelCreateInfo> dataArray = {
		{
			.filename = AppConfig::ModelFolder + "Sponza/Sponza.gltf",
			.instanceCount = 1,
			.playAnimation = false
		}
	};
	bool supportDeviceAddress = true;
	scene_ = std::make_unique<Scene>(vulkanContext_, dataArray, supportDeviceAddress);

	// Pipelines
	AddPipeline<PipelineClear>(vulkanContext_); // This is responsible to clear swapchain image
	AddPipeline<PipelineSkybox>(
		vulkanContext_,
		&(resourcesIBL_->environmentCubemap_),
		resourcesShared_,
		// This is the first offscreen render pass so we need to clear the color attachment and depth attachment
		RenderPassBit::ColorClear | RenderPassBit::DepthClear);
	aabbPtr_ = AddPipeline<PipelineAABBGenerator>(vulkanContext_, resCF_);
	lightCullPtr_ = AddPipeline<PipelineLightCulling>(vulkanContext_, resourcesLight_, resCF_);
	pbrOpaquePtr_ = AddPipeline<PipelinePBRClusterForward>(
		vulkanContext_,
		scene_.get(),
		resourcesLight_,
		resCF_,
		resourcesIBL_,
		resourcesShared_,
		MaterialType::Opaque);
	pbrTransparentPtr_ = AddPipeline<PipelinePBRClusterForward>(
		vulkanContext_,
		scene_.get(),
		resourcesLight_,
		resCF_,
		resourcesIBL_,
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

	resourcesLight_ = AddResources<ResourcesLight>();
	resourcesLight_->AddLights(vulkanContext_, lights);
}

void AppPBRClusterForward::UpdateUBOs()
{
	// Camera UBO
	CameraUBO ubo = camera_->GetCameraUBO();
	for (auto& pipeline : pipelines_)
	{
		pipeline->SetCameraUBO(vulkanContext_, ubo);
	}

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
	if (!ShowImGui())
	{
		imguiPtr_->ImGuiDrawEmpty();
		return;
	}

	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("Clustered Forward Shading", 450, 350);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);
	ImGui::Checkbox("Render Lights", &uiData_.renderLights_);
	imguiPtr_->ImGuiShowPBRConfig(&uiData_.pbrPC_, resourcesIBL_->cubemapMipmapCount_);
	imguiPtr_->ImGuiEnd();

	for (auto& pipeline : pipelines_)
	{
		pipeline->UpdateFromIUData(vulkanContext_, uiData_);
	}
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

	// Destroy all objects
	scene_.reset();

	DestroyResources();
}