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
	VkSampleCountFlagBits msaaSamples = vulkanDevice.GetMSAASamples();

	model_ = std::make_unique<Model>(
		vulkanDevice, 
		AppSettings::ModelFolder + "Tachikoma//Tachikoma.gltf");
	std::vector<Model*> models = {model_.get()};

	// Create a cubemap from the input HDR
	{
		RendererEquirect2Cube e2c(
			vulkanDevice,
			hdrFile);
		e2c.OffscreenRender(vulkanDevice,
			&environmentCubemap_); // Output
	}

	// Cube filtering
	{
		RendererCubeFilter cubeFilter(vulkanDevice, &environmentCubemap_);
		// Diffuse
		cubeFilter.OffscreenRender(vulkanDevice,
			&diffuseCubemap_,
			CubeFilterType::Diffuse);
		// Specular
		cubeFilter.OffscreenRender(vulkanDevice,
			&specularCubemap_,
			CubeFilterType::Specular);
	}
	
	// BRDF look up table
	{
		RendererBRDFLUT brdfLUTCompute(vulkanDevice);
		brdfLUTCompute.CreateLUT(vulkanDevice, &brdfLut_);
	}

	uint32_t width = static_cast<uint32_t>(AppSettings::ScreenWidth);
	uint32_t height = static_cast<uint32_t>(AppSettings::ScreenHeight);

	// Depth attachment (OnScreen and offscreen)
	depthImage_.CreateDepthResources(
		vulkanDevice,
		width,
		height,
		msaaSamples);

	// Color attachments
	// Multi-sampled
	multiSampledColorImage_.CreateColorResources(
		vulkanDevice,
		width,
		height,
		msaaSamples);
	// Single-sampled
	singleSampledColorImage_.CreateColorResources(
		vulkanDevice,
		width,
		height);

	// Renderers
	// This is responsible to clear depth and swapchain image
	clearPtr_ = std::make_unique<RendererClear>(
		vulkanDevice);
	// This draws a cube
	skyboxPtr_ = std::make_unique<RendererSkybox>(
		vulkanDevice,
		&environmentCubemap_,
		&depthImage_,
		&multiSampledColorImage_,
		// This is the first offscreen render pass so
		// we need to clear the color attachment and depth attachment
		RenderPassBit::OffScreenColorClear | 
		RenderPassBit::OffScreenDepthClear
	);
	// This draws meshes with PBR+IBL
	pbrPtr_ = std::make_unique<RendererPBR>(
		vulkanDevice,
		models,
		&depthImage_,
		&specularCubemap_,
		&diffuseCubemap_,
		&brdfLut_,
		&multiSampledColorImage_);
	// Resolve multiSampledColorImage_ to singleSampledColorImage_
	multisampleResolvePtr = std::make_unique<RendererResolveMS>(
		vulkanDevice, &multiSampledColorImage_, &singleSampledColorImage_);
	// This is OnScreen render pass that transfers singleSampledColorImage_ to swapchain image
	tonemapPtr_ = std::make_unique<RendererTonemap>(
		vulkanDevice,
		&singleSampledColorImage_
	);
	// Present swapchain image
	finishPtr_ = std::make_unique<RendererFinish>(
		vulkanDevice);

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
	depthImage_.Destroy(vulkanDevice.GetDevice());
	multiSampledColorImage_.Destroy(vulkanDevice.GetDevice());
	singleSampledColorImage_.Destroy(vulkanDevice.GetDevice());
	environmentCubemap_.Destroy(vulkanDevice.GetDevice());
	diffuseCubemap_.Destroy(vulkanDevice.GetDevice());
	specularCubemap_.Destroy(vulkanDevice.GetDevice());
	brdfLut_.Destroy(vulkanDevice.GetDevice());

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
		.cameraProjection = camera->GetProjectionMatrix(),
		.cameraView = glm::mat4(glm::mat3(camera->GetViewMatrix())), // Remove translation
		.cameraPosition = glm::vec4(camera->Position(), 1.f)
	};
	skyboxPtr_->SetPerFrameUBO(vulkanDevice, imageIndex, skyboxUBO);
	PerFrameUBO pbrUBO
	{
		.cameraProjection = camera->GetProjectionMatrix(),
		.cameraView = camera->GetViewMatrix(),
		.cameraPosition = glm::vec4(camera->Position(), 1.f)
	};
	pbrPtr_->SetPerFrameUBO(vulkanDevice, imageIndex, pbrUBO);

	// Model UBOs
	glm::mat4 modelMatrix(1.f);
	modelMatrix = glm::rotate(modelMatrix, modelRotation_, glm::vec3(0.f, 1.f, 0.f));
	modelRotation_ += deltaTime * 0.1f;

	// 1
	ModelUBO modelUBO1
	{
		.model = modelMatrix
	};
	model_->SetModelUBO(vulkanDevice, imageIndex, modelUBO1);
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