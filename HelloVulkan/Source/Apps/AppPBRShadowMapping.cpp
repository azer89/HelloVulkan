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
	CreateSharedImageResources();

	std::string hdrFile = AppConfig::TextureFolder + "piazza_bologni_1k.hdr";

	sponzaModel_ = std::make_unique<Model>(
		vulkanContext_, 
		AppConfig::ModelFolder + "Sponza//Sponza.gltf");
	tachikomaModel_ = std::make_unique<Model>(
		vulkanContext_,
		AppConfig::ModelFolder + "Tachikoma//Tachikoma.gltf");
	std::vector<Model*> models = {sponzaModel_.get(), tachikomaModel_.get()};

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
		&shadowMap_,
		&depthImage_,
		&multiSampledColorImage_);
	shadowPtr_ = std::make_unique<PipelineShadow>(vulkanContext_, models, &shadowMap_);
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
		{
			.color_ = glm::vec4(1.f),
			.radius_ = 1.0f
		},

		// Add additional lights so that the scene is not too dark
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
	shadowMap_.Destroy();
	environmentCubemap_.Destroy();
	diffuseCubemap_.Destroy();
	specularCubemap_.Destroy();
	brdfLut_.Destroy();

	// Destroy meshes
	sponzaModel_.reset();
	tachikomaModel_.reset();

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

	// Sponza
	glm::mat4 modelMatrix(1.f);
	sponzaModel_->SetModelUBO(vulkanContext_, 
	{
		.model = modelMatrix
	});

	// Tachikoma
	modelMatrix = glm::mat4(1.f);
	modelMatrix = glm::rotate(modelMatrix, glm::radians(45.f), glm::vec3(0.f, 1.f, 0.f));
	modelMatrix = glm::translate(modelMatrix, glm::vec3(-0.5f, 0.62f, 0.f));
	tachikomaModel_->SetModelUBO(vulkanContext_, 
	{
		.model = modelMatrix
	});

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
		.lightIntensity = 0.5f,
		.lightFalloff = 0.1f
	};
	static ShadowMapUBO staticShadowUBO =
	{
		.shadowMinBias = 0.001f,
		.shadowMaxBias = 0.001f,
		.shadowNearPlane = 15.0f,
		.shadowFarPlane = 50.0f
	};
	static float staticLightPos[3] = { -5.f, 45.0f, 5.0f};
	static int staticPCFIteration = 1;

	imguiPtr_->StartImGui();

	ImGui::SetNextWindowSize(ImVec2(525, 500));
	ImGui::Begin(AppConfig::ScreenTitle.c_str());
	ImGui::SetWindowFontScale(1.25f);
	
	ImGui::Text("FPS : %.0f", (1.f / deltaTime_));

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
	ImGui::SliderInt("PCF Iteration", &staticPCFIteration, 1, 10);

	ImGui::SeparatorText("Light position");
	ImGui::SliderFloat("X", &(staticLightPos[0]), -10.0f, 10.0f);
	ImGui::SliderFloat("Y", &(staticLightPos[1]), 40.0f, 70.0f);
	ImGui::SliderFloat("Z", &(staticLightPos[2]), -10.0f, 10.0f);

	imguiPtr_->EndImGui();

	lights_.UpdateLightPosition(vulkanContext_, 0, &(staticLightPos[0]));

	lightPtr_->RenderEnable(staticLightRender);
	pbrPtr_->SetPBRPushConstants(staticPBRPushConstants);

	shadowUBO_.shadowMinBias = staticShadowUBO.shadowMinBias;
	shadowUBO_.shadowMaxBias = staticShadowUBO.shadowMaxBias;
	shadowUBO_.shadowNearPlane = staticShadowUBO.shadowNearPlane;
	shadowUBO_.shadowFarPlane = staticShadowUBO.shadowFarPlane;
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