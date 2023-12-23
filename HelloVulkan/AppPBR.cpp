#include "AppPBR.h"
#include "AppSettings.h"
#include "VulkanUtility.h"
#include "RendererEquirect2Cube.h"
#include "RendererCubeFilter.h"

#include "glm/gtc/matrix_transform.hpp"

AppPBR::AppPBR() :
	modelRotation(0.f)
{
}

int AppPBR::MainLoop()
{
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

	VulkanImage depthImage;
	depthImage.CreateDepthResources(vulkanDevice, 
		static_cast<uint32_t>(AppSettings::ScreenWidth),
		static_cast<uint32_t>(AppSettings::ScreenHeight));
	
	// Create a cubemap from the input HDR
	VulkanTexture envMap;
	{
		RendererEquirect2Cube e2c(
			vulkanDevice, 
			cubemapTextureFile);
		e2c.OffscreenRender(vulkanDevice, 
			&envMap); // Output
	}

	// Diffuse cubemap
	VulkanTexture diffuseMap;
	{
		RendererCubeFilter cFilter(vulkanDevice, &envMap);
		cFilter.OffscreenRender(vulkanDevice,
			&envMap,
			&diffuseMap);
	}

	// Renderers
	clearPtr = std::make_unique<RendererClear>(vulkanDevice, &depthImage);
	finishPtr = std::make_unique<RendererFinish>(vulkanDevice, &depthImage);
	pbrPtr = std::make_unique<RendererPBR>(
		vulkanDevice,
		&depthImage,
		&envMap,
		&diffuseMap,
		meshInfos);
	skyboxPtr = std::make_unique<RendererSkybox>(vulkanDevice, &envMap, &depthImage);

	const std::vector<RendererBase*> renderers = 
	{ 
		clearPtr.get(),
		skyboxPtr.get(),
		pbrPtr.get(),
		finishPtr.get()
	};

	// Main loop
	while (!GLFWWindowShouldClose())
	{
		PollEvents();
		ProcessTiming();
		ProcessInput();

		DrawFrame(renderers);
	}

	// Destroy resources
	depthImage.Destroy(vulkanDevice.GetDevice());
	envMap.Destroy(vulkanDevice.GetDevice());
	diffuseMap.Destroy(vulkanDevice.GetDevice());
	clearPtr = nullptr;
	finishPtr = nullptr;
	skyboxPtr = nullptr;
	pbrPtr = nullptr;

	Terminate();

	return 0;
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
	skyboxPtr->SetPerFrameUBO(vulkanDevice, imageIndex, skyboxUBO);
	PerFrameUBO pbrUBO
	{
		.cameraProjection = camera->GetProjectionMatrix(),
		.cameraView = camera->GetViewMatrix(),
		.cameraPosition = glm::vec4(camera->Position, 1.f)
	};
	pbrPtr->SetPerFrameUBO(vulkanDevice, imageIndex, pbrUBO);

	// Model UBOs
	glm::mat4 model(1.f);
	model = glm::translate(model, glm::vec3(-1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	
	// 1
	ModelUBO modelUBO1
	{
		.model = model
	};
	pbrPtr->meshes_[0].SetModelUBO(vulkanDevice, imageIndex, modelUBO1);

	// 2
	model = glm::mat4(1.f);
	model = glm::translate(model, glm::vec3(2.0f, -1.0f, -2.0f));
	ModelUBO modelUBO2
	{
		.model = model
	};
	pbrPtr->meshes_[1].SetModelUBO(vulkanDevice, imageIndex, modelUBO2);

	modelRotation += deltaTime * 0.01f;
}