#include "AppPBR.h"
#include "AppSettings.h"
#include "VulkanUtility.h"
#include "MeshCreateInfo.h"

#include "glm/gtc/matrix_transform.hpp"

AppPBR::AppPBR() :
	modelRotation(0.f)
{
}

int AppPBR::MainLoop()
{
	std::string cubemapTextureFile = AppSettings::TextureFolder + "the_sky_is_on_fire_4k.hdr";
	std::string cubemapIrradianceFile = AppSettings::TextureFolder + "the_sky_is_on_fire_4k_irradiance.hdr";

	MeshCreateInfo meshInfo
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
	std::vector<MeshCreateInfo> meshInfos;
	meshInfos.push_back(meshInfo);
	meshInfos.push_back(meshInfo);

	VulkanImage depthTexture;
	depthTexture.CreateDepthResources(vulkanDevice, 
		static_cast<uint32_t>(AppSettings::ScreenWidth),
		static_cast<uint32_t>(AppSettings::ScreenHeight));
	
	skyboxPtr = std::make_unique<RendererSkybox>(vulkanDevice, depthTexture, cubemapTextureFile.c_str());
	clearPtr = std::make_unique<RendererClear>(vulkanDevice, depthTexture);
	finishPtr = std::make_unique<RendererFinish>(vulkanDevice, depthTexture);
	pbrPtr = std::make_unique<RendererPBR>(
		vulkanDevice,
		meshInfos,
		cubemapTextureFile.c_str(),
		cubemapIrradianceFile.c_str(),
		depthTexture);

	const std::vector<RendererBase*> renderers = 
	{ 
		clearPtr.get(),
		skyboxPtr.get(),
		pbrPtr.get(),
		finishPtr.get()
	};

	while (!GLFWWindowShouldClose())
	{
		PollEvents();
		ProcessTiming();
		ProcessInput();

		DrawFrame(renderers);
	}

	depthTexture.Destroy(vulkanDevice.GetDevice());

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
	model = glm::rotate(model, modelRotation, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	
	
	// 1
	ModelUBO modelUBO1
	{
		.model = model
	};
	pbrPtr->meshes_[0].SetModelUBO(vulkanDevice, imageIndex, modelUBO1);

	// 2
	model = glm::mat4(1.f);
	model = glm::translate(model, glm::vec3(3.0f, 0.0f, 0.0f));
	model = glm::rotate(model, -modelRotation, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	ModelUBO modelUBO2
	{
		.model = model
	};
	pbrPtr->meshes_[1].SetModelUBO(vulkanDevice, imageIndex, modelUBO2);

	modelRotation += deltaTime * 0.01f;
}