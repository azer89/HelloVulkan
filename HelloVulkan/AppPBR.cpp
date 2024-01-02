#include "AppPBR.h"
#include "AppSettings.h"
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
	std::string hdrFile = AppSettings::TextureFolder + "piazza_bologni_1k.hdr";

	// MSAA
	VkSampleCountFlagBits msaaSamples = vulkanDevice_.GetMSAASamples();

	model_ = std::make_unique<Model>(
		vulkanDevice_, 
		AppSettings::ModelFolder + "Tachikoma//Tachikoma.gltf");
	std::vector<Model*> models = {model_.get()};

	// Create a cubemap from the input HDR
	{
		RendererEquirect2Cube e2c(
			vulkanDevice_,
			hdrFile);
		e2c.OffscreenRender(vulkanDevice_,
			&environmentCubemap_); // Output
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
	}
	
	// BRDF look up table
	{
		RendererBRDFLUT brdfLUTCompute(vulkanDevice_);
		brdfLUTCompute.CreateLUT(vulkanDevice_, &brdfLut_);
	}

	uint32_t width = static_cast<uint32_t>(AppSettings::ScreenWidth);
	uint32_t height = static_cast<uint32_t>(AppSettings::ScreenHeight);

	// Depth attachment (OnScreen and offscreen)
	depthImage_.CreateDepthResources(
		vulkanDevice_,
		width,
		height,
		msaaSamples);

	// Color attachments
	// Multi-sampled
	multiSampledColorImage_.CreateColorResources(
		vulkanDevice_,
		width,
		height,
		msaaSamples);
	// Single-sampled
	singleSampledColorImage_.CreateColorResources(
		vulkanDevice_,
		width,
		height);

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
		&specularCubemap_,
		&diffuseCubemap_,
		&brdfLut_,
		&depthImage_,
		&multiSampledColorImage_);
	// Resolve multiSampledColorImage_ to singleSampledColorImage_
	multisampleResolvePtr = std::make_unique<RendererResolveMS>(
		vulkanDevice_, &multiSampledColorImage_, &singleSampledColorImage_);
	// This is OnScreen render pass that transfers 
	// singleSampledColorImage_ to swapchain image
	tonemapPtr_ = std::make_unique<RendererTonemap>(
		vulkanDevice_,
		&singleSampledColorImage_
	);
	// Present swapchain image
	finishPtr_ = std::make_unique<RendererFinish>(vulkanDevice_);

	// Put all renderer pointers to a vector
	renderers_ =
	{
		// Must be in order
		clearPtr_.get(),
		skyboxPtr_.get(),
		pbrPtr_.get(),
		multisampleResolvePtr.get(),
		tonemapPtr_.get(),
		finishPtr_.get()
	};
}

void AppPBR::DestroyResources()
{
	// Destroy images
	depthImage_.Destroy(vulkanDevice_.GetDevice());
	multiSampledColorImage_.Destroy(vulkanDevice_.GetDevice());
	singleSampledColorImage_.Destroy(vulkanDevice_.GetDevice());
	environmentCubemap_.Destroy(vulkanDevice_.GetDevice());
	diffuseCubemap_.Destroy(vulkanDevice_.GetDevice());
	specularCubemap_.Destroy(vulkanDevice_.GetDevice());
	brdfLut_.Destroy(vulkanDevice_.GetDevice());

	// Destroy meshes
	model_.reset();

	// Destroy renderers
	clearPtr_.reset();
	finishPtr_.reset();
	skyboxPtr_.reset();
	pbrPtr_.reset();
	multisampleResolvePtr.reset();
	tonemapPtr_.reset();
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
	pbrPtr_->SetPerFrameUBO(vulkanDevice_, imageIndex, pbrUBO);

	// Model UBOs
	glm::mat4 modelMatrix(1.f);
	modelMatrix = glm::rotate(modelMatrix, modelRotation_, glm::vec3(0.f, 1.f, 0.f));
	modelRotation_ += deltaTime_ * 0.1f;

	// 1
	ModelUBO modelUBO1
	{
		.model = modelMatrix
	};
	model_->SetModelUBO(vulkanDevice_, imageIndex, modelUBO1);
}

int AppPBR::MainLoop()
{
	Init();

	// Main loop
	while (!GLFWWindowShouldClose())
	{
		PollEvents();
		ProcessTiming();
		ProcessInput();

		DrawFrame(renderers_);
	}

	DestroyResources();

	Terminate();

	return 0;
}