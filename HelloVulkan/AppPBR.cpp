#include "AppPBR.h"
#include "AppSettings.h"
#include "RendererEquirect2Cube.h"
#include "RendererCubeFilter.h"
#include "RendererBRDFLUT.h"

#include "glm/gtc/matrix_transform.hpp"

AppPBR::AppPBR()
{
}

void AppPBR::Init()
{
	modelRotation_ = 0.f;

	std::string hdrFile = AppSettings::TextureFolder + "piazza_bologni_1k.hdr";

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

	// Depth attachment (OnScreen and offscreen)
	depthImage_.CreateDepthResources(vulkanDevice,
		static_cast<uint32_t>(AppSettings::ScreenWidth),
		static_cast<uint32_t>(AppSettings::ScreenHeight));

	// Color attachment (OffScreen only)
	colorImage_.CreateColorResources(vulkanDevice,
		static_cast<uint32_t>(AppSettings::ScreenWidth),
		static_cast<uint32_t>(AppSettings::ScreenHeight));

	// Renderers
	// This is responsible to clear depth and swapchain image
	clearPtr_ = std::make_unique<RendererClear>(
		vulkanDevice, 
		&depthImage_);
	// This draws a cube
	skyboxPtr_ = std::make_unique<RendererSkybox>(
		vulkanDevice,
		&environmentCubemap_,
		&depthImage_,
		&colorImage_,
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
		&colorImage_,
		// This is the last offscreen render pass
		// so transition color attachment to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		RenderPassBit::OffScreenColorShaderReadOnly);
	// This is OnScreen render pass that transfers colorImage_ to swapchain image
	tonemapPtr_ = std::make_unique<RendererTonemap>(
		vulkanDevice,
		&colorImage_,
		&depthImage_
	);
	// This is responsible to present swapchain image
	finishPtr_ = std::make_unique<RendererFinish>(
		vulkanDevice, 
		&depthImage_);

	renderers_ =
	{
		// Must be in order
		clearPtr_.get(),
		skyboxPtr_.get(),
		pbrPtr_.get(),
		tonemapPtr_.get(),
		finishPtr_.get()
	};
}

void AppPBR::DestroyResources()
{
	// Destroy images
	depthImage_.Destroy(vulkanDevice.GetDevice());
	colorImage_.Destroy(vulkanDevice.GetDevice());
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