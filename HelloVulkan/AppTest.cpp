#include "AppTest.h"
#include "AppSettings.h"
#include "VulkanImage.h"
#include "VulkanUtility.h"

AppTest::AppTest()
{
}

int AppTest::MainLoop()
{
	VulkanImage depthTexture;
	depthTexture.CreateDepthResources(vulkanDevice, 
		static_cast<uint32_t>(AppSettings::ScreenWidth),
		static_cast<uint32_t>(AppSettings::ScreenHeight));
	std::string cubemapTextureFile = AppSettings::TextureFolder + "the_sky_is_on_fire_4k.hdr";
	cubePtr = std::make_unique<RendererCube>(vulkanDevice, depthTexture, cubemapTextureFile.c_str());
	clearPtr = std::make_unique<RendererClear>(vulkanDevice, depthTexture);
	finishPtr = std::make_unique<RendererFinish>(vulkanDevice, depthTexture);

	const std::vector<RendererBase*> renderers = 
	{ 
		clearPtr.get(),
		cubePtr.get(),
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
	Terminate();

	return 0;
}

void AppTest::ComposeFrame(uint32_t imageIndex, const std::vector<RendererBase*>& renderers)
{
	// Renderer
	glm::mat4 model(1.f);
	glm::mat4 projection = camera->GetProjectionMatrix();
	glm::mat4 view = camera->GetViewMatrix();
	cubePtr->UpdateUniformBuffer(vulkanDevice, imageIndex, projection * view * model);

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