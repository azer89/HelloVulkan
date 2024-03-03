#include "AppPBRSlotBased.h"
#include "Configs.h"
#include "VulkanUtility.h"
#include "PipelineEquirect2Cube.h"
#include "PipelineCubeFilter.h"
#include "PipelineBRDFLUT.h"

#include "glm/gtc/matrix_transform.hpp"

#include "imgui_impl_glfw.h"
#include "imgui_impl_volk.h"

AppPBRSlotBased::AppPBRSlotBased() :
	modelRotation_(0.f)
{
}

void AppPBRSlotBased::Init()
{
	// Initialize lights
	InitLights();

	// Initialize attachments
	InitSharedImageResources();

	// Image-Based Lighting
	InitIBLResources(AppConfig::TextureFolder + "piazza_bologni_1k.hdr");
	cubemapMipmapCount_ = static_cast<float>(Utility::MipMapCount(IBLConfig::InputCubeSideLength));

	model_ = std::make_unique<Model>();
	model_->LoadSlotBased(vulkanContext_,
		AppConfig::ModelFolder + "DamagedHelmet//DamagedHelmet.gltf");
	std::vector<Model*> models = { model_.get() };

	// Pipelines
	// This is responsible to clear swapchain image
	clearPtr_ = std::make_unique<PipelineClear>(
		vulkanContext_);
	// This draws a cube
	skyboxPtr_ = std::make_unique<PipelineSkybox>(
		vulkanContext_,
		&(resIBL_->environmentCubemap_),
		&(resShared_->depthImage_),
		&(resShared_->multiSampledColorImage_),
		// This is the first offscreen render pass so
		// we need to clear the color attachment and depth attachment
		RenderPassBit::ColorClear |
		RenderPassBit::DepthClear
	);
	pbrPtr_ = std::make_unique<PipelinePBRSlotBased>(
		vulkanContext_,
		models,
		lights_.get(),
		resIBL_.get(),
		&(resShared_->depthImage_),
		&(resShared_->multiSampledColorImage_));
	lightPtr_ = std::make_unique<PipelineLightRender>(
		vulkanContext_,
		lights_.get(),
		&(resShared_->depthImage_),
		&(resShared_->multiSampledColorImage_)
	);
	// Resolve multiSampledColorImage_ to singleSampledColorImage_
	resolveMSPtr_ = std::make_unique<PipelineResolveMS>(
		vulkanContext_, &(resShared_->multiSampledColorImage_), &(resShared_->singleSampledColorImage_));
	// This is on-screen render pass that transfers 
	// singleSampledColorImage_ to swapchain image
	tonemapPtr_ = std::make_unique<PipelineTonemap>(
		vulkanContext_, &(resShared_->singleSampledColorImage_)
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
		pbrPtr_.get(),
		lightPtr_.get(),
		resolveMSPtr_.get(),
		tonemapPtr_.get(),
		imguiPtr_.get(),
		finishPtr_.get()
	};
}

void AppPBRSlotBased::InitLights()
{
	// Lights (SSBO)
	lights_ = std::make_unique<Lights>();
	lights_->AddLights(vulkanContext_,
		{
			{.position_ = glm::vec4(-1.5f, 0.7f,  1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
			{.position_ = glm::vec4(1.5f, 0.7f,  1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
			{.position_ = glm::vec4(-1.5f, 0.7f, -1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
			{.position_ = glm::vec4(1.5f, 0.7f, -1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f }
		});
}

void AppPBRSlotBased::DestroyResources()
{
	// Resources
	resIBL_.reset();
	lights_.reset();

	// Destroy meshes
	model_->Destroy();
	model_.reset();
	
	// Destroy renderers
	clearPtr_.reset();
	finishPtr_.reset();
	skyboxPtr_.reset();
	pbrPtr_.reset();
	lightPtr_.reset();
	resolveMSPtr_.reset();
	tonemapPtr_.reset();
	imguiPtr_.reset();
}

void AppPBRSlotBased::UpdateUBOs()
{
	CameraUBO ubo = camera_->GetCameraUBO();
	lightPtr_->SetCameraUBO(vulkanContext_, ubo);
	pbrPtr_->SetCameraUBO(vulkanContext_, ubo);

	// Remove translation
	CameraUBO skyboxUbo = ubo;
	skyboxUbo.view = glm::mat4(glm::mat3(skyboxUbo.view));
	skyboxPtr_->SetCameraUBO(vulkanContext_, skyboxUbo);

	// Model UBOs
	glm::mat4 modelMatrix(1.f);
	modelMatrix = glm::rotate(modelMatrix, modelRotation_, glm::vec3(0.f, 1.f, 0.f));
	//modelRotation_ += deltaTime_ * 0.1f;

	ModelUBO modelUBO1
	{
		.model = modelMatrix
	};
	model_->SetModelUBO(vulkanContext_, modelUBO1);
}

void AppPBRSlotBased::UpdateUI()
{
	if (!showImgui_)
	{
		imguiPtr_->ImGuiDrawEmpty();
		return;
	}

	static bool lightRender = true;
	static PushConstPBR pbrPC;

	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("PBR and IBL", 525, 350);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);
	ImGui::Checkbox("Render Lights", &lightRender);
	imguiPtr_->ImGuiShowPBRConfig(&pbrPC, cubemapMipmapCount_);
	imguiPtr_->ImGuiEnd();

	lightPtr_->RenderEnable(lightRender);
	pbrPtr_->SetPBRPushConstants(pbrPC);
}

// This is called from main.cpp
int AppPBRSlotBased::MainLoop()
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
	Destroy();

	return 0;
}