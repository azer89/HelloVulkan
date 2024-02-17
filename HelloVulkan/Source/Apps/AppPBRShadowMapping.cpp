#include "AppPBRShadowMapping.h"
#include "Configs.h"
#include "VulkanUtility.h"
#include "PipelineEquirect2Cube.h"
#include "PipelineCubeFilter.h"
#include "PipelineBRDFLUT.h"

#include "glm/gtc/matrix_transform.hpp"

#include "imgui_impl_glfw.h"
#include "imgui_impl_volk.h"

AppPBRShadowMapping::AppPBRShadowMapping() :
	modelRotation_(0.f)
{
}

void AppPBRShadowMapping::Init()
{
	// Initialize lights
	InitLights();

	// Initialize attachments
	CreateSharedImageResources();

	std::string hdrFile = AppConfig::TextureFolder + "piazza_bologni_1k.hdr";

	model_ = std::make_unique<Model>(
		vulkanContext_, 
		AppConfig::ModelFolder + "DamagedHelmet//DamagedHelmet.gltf");
	std::vector<Model*> models = {model_.get()};

	// Create a cubemap from the input HDR
	{
		PipelineEquirect2Cube e2c(
			vulkanContext_,
			hdrFile);
		e2c.OffscreenRender(vulkanContext_,
			&environmentCubemap_); // Output
		environmentCubemap_.SetDebugName(vulkanContext_, "Environment_Cubemap");
	}

	// Cube filtering
	{
		PipelineCubeFilter cubeFilter(vulkanContext_, &environmentCubemap_);
		// Diffuse
		cubeFilter.OffscreenRender(vulkanContext_,
			&diffuseCubemap_,
			CubeFilterType::Diffuse);
		// Specular
		cubeFilter.OffscreenRender(vulkanContext_,
			&specularCubemap_,
			CubeFilterType::Specular);

		diffuseCubemap_.SetDebugName(vulkanContext_, "Diffuse_Cubemap");
		specularCubemap_.SetDebugName(vulkanContext_, "Specular_Cubemap");

		cubemapMipmapCount_ = static_cast<float>(Utility::MipMapCount(IBLConfig::InputCubeSideLength));
	}
	
	// BRDF look up table
	{
		PipelineBRDFLUT brdfLUTCompute(vulkanContext_);
		brdfLUTCompute.CreateLUT(vulkanContext_, &brdfLut_);
		brdfLut_.SetDebugName(vulkanContext_, "BRDF_LUT");
	}

	// Renderers
	// This is responsible to clear swapchain image
	clearPtr_ = std::make_unique<PipelineClear>(
		vulkanContext_);
	// This draws a cube
	skyboxPtr_ = std::make_unique<PipelineSkybox>(
		vulkanContext_,
		&environmentCubemap_,
		&depthImage_,
		&multiSampledColorImage_,
		// This is the first offscreen render pass so
		// we need to clear the color attachment and depth attachment
		RenderPassBit::ColorClear | 
		RenderPassBit::DepthClear
	);
	pbrPtr_ = std::make_unique<PipelinePBRShadowMapping>(
		vulkanContext_,
		models,
		&lights_,
		&specularCubemap_,
		&diffuseCubemap_,
		&brdfLut_,
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

void AppPBRShadowMapping::InitLights()
{
	// Lights (SSBO)
	lights_.AddLights(vulkanContext_,
	{
		{
			.position_ = glm::vec4(-1.5f, 0.7f,  1.5f, 1.f),
			.color_ = glm::vec4(1.f),
			.radius_ = 10.0f
		},
		{
			.position_ = glm::vec4(1.5f, 0.7f,  1.5f, 1.f),
			.color_ = glm::vec4(1.f),
			.radius_ = 10.0f
		},
		{
			.position_ = glm::vec4(-1.5f, 0.7f, -1.5f, 1.f),
			.color_ = glm::vec4(1.f),
			.radius_ = 10.0f
		},
		{
			.position_ = glm::vec4(1.5f, 0.7f, -1.5f, 1.f),
			.color_ = glm::vec4(1.f),
			.radius_ = 10.0f
		}
	});
}

void AppPBRShadowMapping::DestroyResources()
{
	// Destroy images
	environmentCubemap_.Destroy();
	diffuseCubemap_.Destroy();
	specularCubemap_.Destroy();
	brdfLut_.Destroy();

	// Destroy meshes
	model_.reset();

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

void AppPBRShadowMapping::UpdateUBOs()
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

void AppPBRShadowMapping::UpdateUI()
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