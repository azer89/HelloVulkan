#include "AppTest.h"
#include "AppSettings.h"
#include "VulkanImage.h"
#include "VulkanUtility.h"

struct UBO
{
	glm::mat4 mvp;
	glm::mat4 mv;
	glm::mat4 m;
	glm::vec4 cameraPos;
} ubo;

AppTest::AppTest()
{
}

int AppTest::MainLoop()
{
	std::string cubemapTextureFile = AppSettings::TextureFolder + "the_sky_is_on_fire_4k.hdr";
	std::string cubemapIrradianceFile = AppSettings::TextureFolder + "the_sky_is_on_fire_4k_irradiance.hdr";

	std::string gltfFile = AppSettings::ModelFolder + "Dragon//Dragon.obj";
	std::string aoFile = AppSettings::TextureFolder + "pbr//rusted_iron//ao.png";
	std::string emissiveFile = AppSettings::TextureFolder + "Black1x1.png";
	std::string albedoFile = AppSettings::TextureFolder + "pbr//rusted_iron//albedo.png";
	std::string roughnessFile = AppSettings::TextureFolder + "pbr//rusted_iron//roughness.png";
	std::string normalFile = AppSettings::TextureFolder + "pbr//rusted_iron//normal.png";

	VulkanImage depthTexture;
	depthTexture.CreateDepthResources(vulkanDevice, 
		static_cast<uint32_t>(AppSettings::ScreenWidth),
		static_cast<uint32_t>(AppSettings::ScreenHeight));
	
	cubePtr = std::make_unique<RendererCube>(vulkanDevice, depthTexture, cubemapTextureFile.c_str());
	clearPtr = std::make_unique<RendererClear>(vulkanDevice, depthTexture);
	finishPtr = std::make_unique<RendererFinish>(vulkanDevice, depthTexture);
	pbrPtr = std::make_unique<RendererPBR>(vulkanDevice,
		(uint32_t)sizeof(UBO),
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

		const bool frameRendered = DrawFrame(renderers);
	}

	depthTexture.Destroy(vulkanDevice.GetDevice());

	clearPtr = nullptr;
	finishPtr = nullptr;
	cubePtr = nullptr;
	pbrPtr = nullptr;
	Terminate();

	return 0;
}

void AppTest::ComposeFrame(uint32_t imageIndex, const std::vector<RendererBase*>& renderers)
{
	// Renderer
	glm::mat4 model(1.f);
	glm::mat4 projection = camera->GetProjectionMatrix();
	glm::mat4 view = camera->GetViewMatrix();

	glm::mat4 cubeView = glm::mat4(glm::mat3(view)); // Remove translation from the view matrix
	cubePtr->UpdateUniformBuffer(vulkanDevice, imageIndex, projection * cubeView * model);

	model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	ubo = UBO
	{
		.mvp = projection * view * model,
		.mv = view * view,
		.m = model,
		.cameraPos = glm::vec4(camera->Position, 1.f) };
	pbrPtr->UpdateUniformBuffer(vulkanDevice, imageIndex, &ubo, sizeof(ubo));

	VkCommandBuffer commandBuffer = vulkanDevice.commandBuffers[imageIndex];

	const VkCommandBufferBeginInfo bi =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		.pInheritanceInfo = nullptr
	};

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &bi));

	for (auto& r : renderers)
		r->FillCommandBuffer(commandBuffer, imageIndex);

	VK_CHECK(vkEndCommandBuffer(commandBuffer));
}