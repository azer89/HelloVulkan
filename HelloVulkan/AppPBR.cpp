#include "AppPBR.h"
#include "AppSettings.h"
#include "RendererEquirect2Cube.h"
#include "RendererCubeFilter.h"
#include "RendererBRDF.h"

#include "glm/gtc/matrix_transform.hpp"

AppPBR::AppPBR()
{
}

void AppPBR::Init()
{
	modelRotation_ = 0.f;

	std::string cubemapTextureFile = AppSettings::TextureFolder + "piazza_bologni_1k.hdr";

	model_ = std::make_unique<Model>(
		vulkanDevice, 
		AppSettings::ModelFolder + "DamagedHelmet//DamagedHelmet.gltf");
	std::vector<Model*> models = {model_.get()};

	// Create a cubemap from the input HDR
	{
		RendererEquirect2Cube e2c(
			vulkanDevice,
			cubemapTextureFile);
		e2c.OffscreenRender(vulkanDevice,
			&environmentCubemap_); // Output
	}

	// Cube filtering
	{
		RendererCubeFilter cFilter(vulkanDevice, &environmentCubemap_);
		// Diffuse
		cFilter.OffscreenRender(vulkanDevice,
			&diffuseCubemap_,
			DistributionCubeFilter::Lambertian);
		// Specular
		cFilter.OffscreenRender(vulkanDevice,
			&specularCubemap_,
			DistributionCubeFilter::GGX);
	}
	
	{
		RendererBRDF brdfComp(vulkanDevice);
		brdfComp.CreateLUT(vulkanDevice, &lutTexture_);
	}

	depthImage_.CreateDepthResources(vulkanDevice,
		static_cast<uint32_t>(AppSettings::ScreenWidth),
		static_cast<uint32_t>(AppSettings::ScreenHeight));

	// Renderers
	clearPtr_ = std::make_unique<RendererClear>(vulkanDevice, &depthImage_);
	finishPtr_ = std::make_unique<RendererFinish>(vulkanDevice, &depthImage_);
	pbrPtr_ = std::make_unique<RendererPBR>(
		vulkanDevice,
		&depthImage_,
		&specularCubemap_,
		&diffuseCubemap_, 
		&lutTexture_,
		models);
	skyboxPtr_ = std::make_unique<RendererSkybox>(vulkanDevice, &environmentCubemap_, &depthImage_);

	renderers_ =
	{
		clearPtr_.get(),
		skyboxPtr_.get(),
		pbrPtr_.get(),
		finishPtr_.get()
	};
}

void AppPBR::DestroyResources()
{
	// Destroy resources
	depthImage_.Destroy(vulkanDevice.GetDevice());
	environmentCubemap_.Destroy(vulkanDevice.GetDevice());
	diffuseCubemap_.Destroy(vulkanDevice.GetDevice());
	specularCubemap_.Destroy(vulkanDevice.GetDevice());
	lutTexture_.Destroy(vulkanDevice.GetDevice());
	model_.reset();
	clearPtr_.reset();
	finishPtr_.reset();
	skyboxPtr_.reset();
	pbrPtr_.reset();
}

void AppPBR::UpdateUBO(uint32_t imageIndex)
{
	// Per frame UBO
	PerFrameUBO skyboxUBO
	{
		.cameraProjection = camera->GetProjectionMatrix(),
		.cameraView = glm::mat4(glm::mat3(camera->GetViewMatrix())), // Remove translation
		.cameraPosition = glm::vec4(camera->Position, 1.f)
	};
	skyboxPtr_->SetPerFrameUBO(vulkanDevice, imageIndex, skyboxUBO);
	PerFrameUBO pbrUBO
	{
		.cameraProjection = camera->GetProjectionMatrix(),
		.cameraView = camera->GetViewMatrix(),
		.cameraPosition = glm::vec4(camera->Position, 1.f)
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