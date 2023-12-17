#include "AppPBR.h"
#include "AppSettings.h"
#include "VulkanUtility.h"
//#include "VulkanImage.h"

#include "glm/gtc/matrix_transform.hpp"

/*struct UBO
{
	glm::mat4 mvp;
	glm::mat4 mv;
	glm::mat4 m;
	glm::vec4 cameraPos;
} ubo;*/

AppPBR::AppPBR() :
	modelRotation(0.f)
{
}

int AppPBR::MainLoop()
{
	std::string cubemapTextureFile = AppSettings::TextureFolder + "the_sky_is_on_fire_4k.hdr";
	std::string cubemapIrradianceFile = AppSettings::TextureFolder + "the_sky_is_on_fire_4k_irradiance.hdr";

	std::string gltfFile = AppSettings::ModelFolder + "DamagedHelmet//DamagedHelmet.gltf";
	std::string aoFile = AppSettings::ModelFolder + "DamagedHelmet//Default_AO.jpg";
	std::string emissiveFile = AppSettings::ModelFolder + "DamagedHelmet//Default_emissive.jpg";
	std::string albedoFile = AppSettings::ModelFolder + "DamagedHelmet//Default_albedo.jpg";
	std::string roughnessFile = AppSettings::ModelFolder + "DamagedHelmet//Default_metalRoughness.jpg";
	std::string normalFile = AppSettings::ModelFolder + "DamagedHelmet//Default_normal.jpg";

	VulkanImage depthTexture;
	depthTexture.CreateDepthResources(vulkanDevice, 
		static_cast<uint32_t>(AppSettings::ScreenWidth),
		static_cast<uint32_t>(AppSettings::ScreenHeight));
	
	cubePtr = std::make_unique<RendererCube>(vulkanDevice, depthTexture, cubemapTextureFile.c_str());
	clearPtr = std::make_unique<RendererClear>(vulkanDevice, depthTexture);
	finishPtr = std::make_unique<RendererFinish>(vulkanDevice, depthTexture);
	pbrPtr = std::make_unique<RendererPBR>(vulkanDevice,
		(uint32_t)sizeof(PerFrameUBO),
		gltfFile.c_str(),
		aoFile.c_str(),
		emissiveFile.c_str(),
		albedoFile.c_str(),
		roughnessFile.c_str(),
		normalFile.c_str(),
		cubemapTextureFile.c_str(),
		cubemapIrradianceFile.c_str(),
		depthTexture);

	const std::vector<RendererBase*> renderers = 
	{ 
		clearPtr.get(),
		cubePtr.get(),
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
	cubePtr = nullptr;
	pbrPtr = nullptr;
	Terminate();

	return 0;
}

void AppPBR::UpdateUBO(uint32_t imageIndex)
{
	// Skybox
	PerFrameUBO skyboxUBO
	{
		.cameraProjection = camera->GetProjectionMatrix(),
		.cameraView = glm::mat4(glm::mat3(camera->GetViewMatrix())),
		.model = glm::mat4(1.f),
		.cameraPosition = glm::vec4(camera->Position, 1.f)
	};

	// Model
	cubePtr->SetUBO(vulkanDevice, imageIndex, skyboxUBO);
	glm::mat4 model(1.f);
	model = glm::rotate(model, modelRotation, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	modelRotation += deltaTime * 0.2f;
	PerFrameUBO meshUBO
	{
		.cameraProjection = camera->GetProjectionMatrix(),
		.cameraView = camera->GetViewMatrix(),
		.model = model,
		.cameraPosition = glm::vec4(camera->Position, 1.f)
	};
	pbrPtr->SetUBO(vulkanDevice, imageIndex, meshUBO);

	/*VkCommandBuffer commandBuffer = vulkanDevice.commandBuffers[imageIndex];

	const VkCommandBufferBeginInfo bi =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		.pInheritanceInfo = nullptr
	};

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &bi));

	for (auto& r : renderers)
	{
		r->FillCommandBuffer(commandBuffer, imageIndex);
	}

	VK_CHECK(vkEndCommandBuffer(commandBuffer));*/
}