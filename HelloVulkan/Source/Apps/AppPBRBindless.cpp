#include "AppPBRBindless.h"
#include "VulkanUtility.h"
#include "Configs.h"

#include "glm/ext.hpp"
#include "imgui_impl_vulkan.h"

AppPBRBindless::AppPBRBindless() :
	modelRotation_(0.f)
{
}

void AppPBRBindless::Init()
{
	camera_->SetPositionAndTarget(glm::vec3(0.0f, 3.0f, 5.0f), glm::vec3(0.0));

	InitLights();

	// Initialize attachments
	InitSharedResources();

	// Image-Based Lighting
	resIBL_ = std::make_unique<ResourcesIBL>(vulkanContext_, AppConfig::TextureFolder + "piazza_bologni_1k.hdr");
	cubemapMipmapCount_ = static_cast<float>(Utility::MipMapCount(IBLConfig::InputCubeSideLength));

	// Scene
	std::vector<ModelCreateInfo> dataArray = { 
		{ AppConfig::ModelFolder + "Sponza/Sponza.gltf", 1},
		{ AppConfig::ModelFolder + "Tachikoma/Tachikoma.gltf", 1},
	};
	bool supportDeviceAddress = true;
	scene_ = std::make_unique<Scene>(vulkanContext_, dataArray, supportDeviceAddress);

	// Tachikoma model matrix
	glm::mat4 modelMatrix(1.f);
	modelMatrix = glm::rotate(modelMatrix, glm::radians(45.f), glm::vec3(0.f, 1.f, 0.f));
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.5f, 0.62f, 0.f));
	scene_->UpdateModelMatrix(vulkanContext_, { .model = modelMatrix }, 1, 0);

	// Pipelines
	// This is responsible to clear swapchain image
	clearPtr_ = std::make_unique<PipelineClear>(vulkanContext_);
	// This draws a cube
	skyboxPtr_ = std::make_unique<PipelineSkybox>(
		vulkanContext_,
		&(resIBL_->diffuseCubemap_),
		resShared_.get(),
		// This is the first offscreen render pass so we need to clear the color attachment and depth attachment
		RenderPassBit::ColorClear | RenderPassBit::DepthClear);
	pbrPtr_ = std::make_unique<PipelinePBRBindless>(
		vulkanContext_,
		scene_.get(),
		resourcesLight_.get(),
		resIBL_.get(),
		resShared_.get());
	lightPtr_ = std::make_unique<PipelineLightRender>(
		vulkanContext_,
		resourcesLight_.get(),
		resShared_.get());
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
		skyboxPtr_.get(),
		pbrPtr_.get(),
		lightPtr_.get(),
		resolveMSPtr_.get(),
		tonemapPtr_.get(),
		imguiPtr_.get(),
		finishPtr_.get()
	};
}

void AppPBRBindless::InitLights()
{
	// Lights (SSBO)
	resourcesLight_ = std::make_unique<ResourcesLight>();
	resourcesLight_->AddLights(vulkanContext_,
	{
		{ .position_ = glm::vec4(-1.5f, 0.7f,  1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{ .position_ = glm::vec4(1.5f, 0.7f,  1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{ .position_ = glm::vec4(-1.5f, 0.7f, -1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{ .position_ = glm::vec4(1.5f, 0.7f, -1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f }
	});
}

void AppPBRBindless::DestroyResources()
{
	// Resources
	resIBL_.reset();
	resourcesLight_.reset();

	// Destroy meshes
	scene_.reset();

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

void AppPBRBindless::UpdateUBOs()
{
	CameraUBO ubo = camera_->GetCameraUBO();
	lightPtr_->SetCameraUBO(vulkanContext_, ubo);
	pbrPtr_->SetCameraUBO(vulkanContext_, ubo);

	// Remove translation
	CameraUBO skyboxUbo = ubo;
	skyboxUbo.view = glm::mat4(glm::mat3(skyboxUbo.view));
	skyboxPtr_->SetCameraUBO(vulkanContext_, skyboxUbo);
}

void AppPBRBindless::UpdateUI()
{
	if (!showImgui_)
	{
		imguiPtr_->ImGuiDrawEmpty();
		return;
	}

	static bool lightRender = true;
	static PushConstPBR pbrPC;

	imguiPtr_->ImGuiStart();
	imguiPtr_->ImGuiSetWindow("Bindless Textures", 525, 350);
	imguiPtr_->ImGuiShowFrameData(&frameCounter_);
	ImGui::Text("Vertices: %i, Indices: %i", scene_->vertices_.size(), scene_->indices_.size());
	ImGui::Checkbox("Render Lights", &lightRender);
	imguiPtr_->ImGuiShowPBRConfig(&pbrPC, cubemapMipmapCount_);
	imguiPtr_->ImGuiEnd();

	lightPtr_->ShouldRender(lightRender);
	pbrPtr_->SetPBRPushConstants(pbrPC);
}

// This is called from main.cpp
void AppPBRBindless::MainLoop()
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

	DestroyResources();
	DestroyInternal();
}