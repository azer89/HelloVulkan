#include "AppPBR.h"
#include "Configs.h"
#include "RendererEquirect2Cube.h"
#include "RendererCubeFilter.h"
#include "RendererBRDFLUT.h"

#include "glm/gtc/matrix_transform.hpp"

AppPBR::AppPBR() :
	modelRotation_(0.f)
{
}

void AppPBR::Init()
{
	// Initialize attachments
	CreateSharedImageResources();

	// Initialize lights
	InitLights();

	std::string hdrFile = AppSettings::TextureFolder + "the_sky_is_on_fire_4k.hdr";

	model_ = std::make_unique<Model>(
		vulkanDevice_, 
		AppSettings::ModelFolder + "DamagedHelmet//DamagedHelmet.gltf");
	std::vector<Model*> models = {model_.get()};

	// Create a cubemap from the input HDR
	{
		RendererEquirect2Cube e2c(
			vulkanDevice_,
			hdrFile);
		e2c.OffscreenRender(vulkanDevice_,
			&environmentCubemap_); // Output
		environmentCubemap_.SetDebugName(vulkanDevice_, "Environment_Cubemap");
	}

	// Cube filtering
	{
		RendererCubeFilter cubeFilter(vulkanDevice_, &environmentCubemap_);
		// Diffuse
		cubeFilter.OffscreenRender(vulkanDevice_,
			&diffuseCubemap_,
			CubeFilterType::Diffuse);
		// Specular
		cubeFilter.OffscreenRender(vulkanDevice_,
			&specularCubemap_,
			CubeFilterType::Specular);

		diffuseCubemap_.SetDebugName(vulkanDevice_, "Diffuse_Cubemap");
		specularCubemap_.SetDebugName(vulkanDevice_, "Specular_Cubemap");
	}
	
	// BRDF look up table
	{
		RendererBRDFLUT brdfLUTCompute(vulkanDevice_);
		brdfLUTCompute.CreateLUT(vulkanDevice_, &brdfLut_);
		brdfLut_.SetDebugName(vulkanDevice_, "BRDF_LUT");
	}

	// Renderers
	// This is responsible to clear swapchain image
	clearPtr_ = std::make_unique<RendererClear>(
		vulkanDevice_);
	// This draws a cube
	skyboxPtr_ = std::make_unique<RendererSkybox>(
		vulkanDevice_,
		&environmentCubemap_,
		&depthImage_,
		&multiSampledColorImage_,
		// This is the first offscreen render pass so
		// we need to clear the color attachment and depth attachment
		RenderPassBit::ColorClear | 
		RenderPassBit::DepthClear
	);
	// This draws meshes with PBR+IBL
	pbrPtr_ = std::make_unique<RendererPBR>(
		vulkanDevice_,
		models,
		&lights_,
		&specularCubemap_,
		&diffuseCubemap_,
		&brdfLut_,
		&depthImage_,
		&multiSampledColorImage_);
	lightPtr_ = std::make_unique<RendererLight>(
		vulkanDevice_,
		&lights_,
		&depthImage_,
		&multiSampledColorImage_
	);
	// Resolve multiSampledColorImage_ to singleSampledColorImage_
	resolveMSPtr_ = std::make_unique<RendererResolveMS>(
		vulkanDevice_, &multiSampledColorImage_, &singleSampledColorImage_);
	// This is on-screen render pass that transfers 
	// singleSampledColorImage_ to swapchain image
	tonemapPtr_ = std::make_unique<RendererTonemap>(
		vulkanDevice_,
		&singleSampledColorImage_
	);
	// ImGui here
	imguiPtr_ = std::make_unique<RendererImGui>(vulkanDevice_, vulkanInstance_.GetInstance(), glfwWindow_);
	// Present swapchain image
	finishPtr_ = std::make_unique<RendererFinish>(vulkanDevice_);

	// Put all renderer pointers to a vector
	renderers_ =
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

void AppPBR::InitLights()
{
	// Lights (SSBO)
	lights_.AddLights(vulkanDevice_,
	{
		{
			.position_ = glm::vec4(-1.5f, 0.7f,  1.5f, 1.f),
			.color_ = glm::vec4(1.f, 0.f, 0.f, 1.f)
		},
		{
			.position_ = glm::vec4(1.5f, 0.7f,  1.5f, 1.f),
			.color_ = glm::vec4(1.f, 0.f, 0.f, 1.f)
		},
		{
			.position_ = glm::vec4(-1.5f, 0.7f, -1.5f, 1.f),
			.color_ = glm::vec4(0.f, 1.f, 0.f, 1.f)
		},
		{
			.position_ = glm::vec4(1.5f, 0.7f, -1.5f, 1.f),
			.color_ = glm::vec4(0.f, 1.f, 0.f, 1.f)
		}
	});
}

void AppPBR::DestroyResources()
{
	// Destroy images
	environmentCubemap_.Destroy(vulkanDevice_.GetDevice());
	diffuseCubemap_.Destroy(vulkanDevice_.GetDevice());
	specularCubemap_.Destroy(vulkanDevice_.GetDevice());
	brdfLut_.Destroy(vulkanDevice_.GetDevice());

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

void AppPBR::UpdateUBOs(uint32_t imageIndex)
{
	// Per frame UBO
	PerFrameUBO skyboxUBO
	{
		.cameraProjection = camera_->GetProjectionMatrix(),
		.cameraView = glm::mat4(glm::mat3(camera_->GetViewMatrix())), // Remove translation
		.cameraPosition = glm::vec4(camera_->Position(), 1.f)
	};
	skyboxPtr_->SetPerFrameUBO(vulkanDevice_, imageIndex, skyboxUBO);
	PerFrameUBO pbrUBO
	{
		.cameraProjection = camera_->GetProjectionMatrix(),
		.cameraView = camera_->GetViewMatrix(),
		.cameraPosition = glm::vec4(camera_->Position(), 1.f)
	};
	lightPtr_->SetPerFrameUBO(vulkanDevice_, imageIndex, pbrUBO);
	pbrPtr_->SetPerFrameUBO(vulkanDevice_, imageIndex, pbrUBO);

	// Model UBOs
	glm::mat4 modelMatrix(1.f);
	modelMatrix = glm::rotate(modelMatrix, modelRotation_, glm::vec3(0.f, 1.f, 0.f));
	//modelRotation_ += deltaTime_ * 0.1f;

	// 1
	ModelUBO modelUBO1
	{
		.model = modelMatrix
	};
	model_->SetModelUBO(vulkanDevice_, imageIndex, modelUBO1);
}

// This is called from main.cpp
int AppPBR::MainLoop()
{
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
	vkDeviceWaitIdle(vulkanDevice_.GetDevice());

	DestroyResources();

	Terminate();

	return 0;
}