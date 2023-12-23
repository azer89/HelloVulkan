#include "AppPBR.h"
#include "AppSettings.h"
#include "RendererEquirect2Cube.h"
#include "RendererCubeFilter.h"

#include "glm/gtc/matrix_transform.hpp"

AppPBR::AppPBR()
{
}

void AppPBR::Init()
{
	modelRotation_ = 0.f;

	std::string cubemapTextureFile = AppSettings::TextureFolder + "the_sky_is_on_fire_4k.hdr";

	MeshCreateInfo meshInfo1
	{
		.modelFile = AppSettings::ModelFolder + "DamagedHelmet//DamagedHelmet.gltf",
		.textureFiles =
		{
			AppSettings::ModelFolder + "DamagedHelmet//Default_AO.jpg",
			AppSettings::ModelFolder + "DamagedHelmet//Default_emissive.jpg",
			AppSettings::ModelFolder + "DamagedHelmet//Default_albedo.jpg",
			AppSettings::ModelFolder + "DamagedHelmet//Default_metalRoughness.jpg",
			AppSettings::ModelFolder + "DamagedHelmet//Default_normal.jpg",
		}
	};

	MeshCreateInfo meshInfo2
	{
		.modelFile = AppSettings::ModelFolder + "Dragon//Dragon.obj",
		.textureFiles =
		{
			AppSettings::TextureFolder + "pbr//plastic//ao.png",
			AppSettings::TextureFolder + "Black1x1.png",
			AppSettings::TextureFolder + "pbr//plastic//albedo.png",
			AppSettings::TextureFolder + "pbr//plastic//roughness.png",
			AppSettings::TextureFolder + "pbr//plastic//normal.png",
		}
	};

	// Creates two meshes for now
	std::vector<MeshCreateInfo> meshInfos;
	meshInfos.push_back(meshInfo1);
	meshInfos.push_back(meshInfo2);

	// Create a cubemap from the input HDR
	{
		RendererEquirect2Cube e2c(
			vulkanDevice,
			cubemapTextureFile);
		e2c.OffscreenRender(vulkanDevice,
			&environmentCubemap_); // Output
	}

	// Diffuse cubemap
	{
		RendererCubeFilter cFilter(vulkanDevice, &environmentCubemap_);
		cFilter.OffscreenRender(vulkanDevice,
			&environmentCubemap_,
			&diffuseCubemap_);
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
		&environmentCubemap_,
		&diffuseCubemap_,
		meshInfos);
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
	clearPtr_ = nullptr;
	finishPtr_ = nullptr;
	skyboxPtr_ = nullptr;
	pbrPtr_ = nullptr;
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
	glm::mat4 model(1.f);
	model = glm::translate(model, glm::vec3(-1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	// 1
	ModelUBO modelUBO1
	{
		.model = model
	};
	pbrPtr_->meshes_[0].SetModelUBO(vulkanDevice, imageIndex, modelUBO1);

	// 2
	model = glm::mat4(1.f);
	model = glm::translate(model, glm::vec3(2.0f, -1.0f, -2.0f));
	ModelUBO modelUBO2
	{
		.model = model
	};
	pbrPtr_->meshes_[1].SetModelUBO(vulkanDevice, imageIndex, modelUBO2);

	modelRotation_ += deltaTime * 0.01f;
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