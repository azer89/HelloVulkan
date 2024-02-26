#include "AppPBRBindless.h"
#include "Configs.h"
#include "VulkanUtility.h"
#include "PipelineEquirect2Cube.h"
#include "PipelineCubeFilter.h"
#include "PipelineBRDFLUT.h"

#include "glm/gtc/matrix_transform.hpp"

#include "imgui_impl_glfw.h"
#include "imgui_impl_volk.h"

AppPBRBindless::AppPBRBindless() :
	modelRotation_(0.f)
{
}

void AppPBRBindless::Init()
{
	// Initialize lights
	InitLights();

	// Initialize attachments
	InitSharedImageResources();

	// Image-Based Lighting
	InitIBLResources(AppConfig::TextureFolder + "piazza_bologni_1k.hdr");
	cubemapMipmapCount_ = static_cast<float>(Utility::MipMapCount(IBLConfig::InputCubeSideLength));

	// Scene
	std::vector<std::string> modelFiles = { 
		AppConfig::ModelFolder + "Sponza//Sponza.gltf",
		AppConfig::ModelFolder + "Tachikoma//Tachikoma.gltf",
	};
	scene_ = std::make_unique<Scene>(vulkanContext_, modelFiles);

	// Tachikoma model matrix
	glm::mat4 modelMatrix(1.f);
	modelMatrix = glm::rotate(modelMatrix, glm::radians(45.f), glm::vec3(0.f, 1.f, 0.f));
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.5f, 0.62f, 0.f));
	for (uint32_t i = 0; i < AppConfig::FrameOverlapCount; ++i)
	{
		scene_->UpdateModelMatrix(vulkanContext_, { .model = modelMatrix }, i, 1);
	}

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
	pbrPtr_ = std::make_unique<PipelinePBRBindless>(
		vulkanContext_,
		scene_.get(),
		&lights_,
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
	lights_.AddLights(vulkanContext_,
	{
		{ .position_ = glm::vec4(-1.5f, 0.7f,  1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{ .position_ = glm::vec4(1.5f, 0.7f,  1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{ .position_ = glm::vec4(-1.5f, 0.7f, -1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f },
		{ .position_ = glm::vec4(1.5f, 0.7f, -1.5f, 1.f), .color_ = glm::vec4(1.f), .radius_ = 10.0f }
	});
}

void AppPBRBindless::DestroyResources()
{
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
int AppPBRBindless::MainLoop()
{
	InitVulkan({
		.supportRaytracing_ = false,
		.supportMSAA_ = true,
		.supportBindlessRendering_ = true,
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