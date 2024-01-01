#include "AppBase.h"
#include "AppSettings.h"
#include "VulkanUtility.h"

#include "volk.h"
#include "glslang_c_interface.h" // Init GLSLang

#include <iostream>

AppBase::AppBase()
{
	InitGLFW();
	InitGLSLang();
	InitVulkan();
	InitIMGUI();
	InitCamera();
	InitTiming();
}

void AppBase::InitVulkan()
{
	// Initialize Volk
	VkResult res = volkInitialize();
	if (res != VK_SUCCESS)
	{
		std::cerr << "Volk Cannot be initialized\n";
	}

	// Initialize Vulkan instance
	vulkanInstance.Create();
	vulkanInstance.SetupDebugCallbacks();
	vulkanInstance.CreateWindowSurface(glfwWindow);
	vulkanDevice.CreateCompute(vulkanInstance,
		static_cast<uint32_t>(AppSettings::ScreenWidth),
		static_cast<uint32_t>(AppSettings::ScreenHeight),
		VkPhysicalDeviceFeatures{});
}

void AppBase::InitGLSLang()
{
	glslang_initialize_process();
}

void AppBase::InitGLFW()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	// GLFW window creation
	glfwWindow = glfwCreateWindow(AppSettings::ScreenWidth,
		AppSettings::ScreenHeight,
		AppSettings::ScreenTitle.c_str(),
		NULL,
		NULL);
	if (glfwWindow == NULL)
	{
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window");
	}
	glfwMakeContextCurrent(glfwWindow);

	glfwSetWindowUserPointer(glfwWindow, this);
	
	auto FuncFramebuffer = [](GLFWwindow* window, int width, int height)
	{
		static_cast<AppBase*>(glfwGetWindowUserPointer(window))->FrameBufferSizeCallback(window, width, height);
	};
	glfwSetFramebufferSizeCallback(glfwWindow, FuncFramebuffer);

	auto FuncCursor = [](GLFWwindow* window, double x, double y)
	{
		static_cast<AppBase*>(glfwGetWindowUserPointer(window))->MouseCallback(window, x, y);
	};
	glfwSetCursorPosCallback(glfwWindow, FuncCursor);

	auto FuncMouse = [](GLFWwindow* window, int button, int action, int mods)
	{
		static_cast<AppBase*>(glfwGetWindowUserPointer(window))->MouseButtonCallback(window, button, action, mods);
	};
	glfwSetMouseButtonCallback(glfwWindow, FuncMouse);

	auto FuncScroll = [](GLFWwindow* window, double xOffset, double yOffset)
	{
		static_cast<AppBase*>(glfwGetWindowUserPointer(window))->ScrollCallback(window, xOffset, yOffset);
	};
	glfwSetScrollCallback(glfwWindow, FuncScroll);

	auto FuncKey = [](GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		static_cast<AppBase*>(glfwGetWindowUserPointer(window))->KeyCallback(window, key, scancode, action, mods);
	};
	glfwSetKeyCallback(glfwWindow, FuncKey);
}

bool AppBase::DrawFrame(const std::vector<RendererBase*>& renderers)
{
	uint32_t imageIndex = 0;
	VkResult result = vkAcquireNextImageKHR(
		vulkanDevice.GetDevice(), 
		vulkanDevice.GetSwapChain(), 
		0, 
		vulkanDevice.GetSemaphore(), 
		VK_NULL_HANDLE, 
		&imageIndex);
	VK_CHECK(vkResetCommandPool(vulkanDevice.GetDevice(), vulkanDevice.GetCommandPool(), 0));

	if (result != VK_SUCCESS)
	{
		return false;
	}

	UpdateUBO(imageIndex);
	UpdateCommandBuffer(renderers, imageIndex);

	const VkPipelineStageFlags waitStages[] = 
		{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	const VkSubmitInfo si =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = vulkanDevice.GetSemaphorePtr(),
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = vulkanDevice.GetCommandBufferPtr(imageIndex),
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = vulkanDevice.GetRenderSemaphorePtr()
	};

	{
		VK_CHECK(vkQueueSubmit(vulkanDevice.GetGraphicsQueue(), 1, &si, nullptr));
	}

	const VkPresentInfoKHR pi =
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = vulkanDevice.GetRenderSemaphorePtr(),
		.swapchainCount = 1,
		.pSwapchains = vulkanDevice.GetSwapchainPtr(),
		.pImageIndices = &imageIndex
	};

	{
		VK_CHECK(vkQueuePresentKHR(vulkanDevice.GetGraphicsQueue(), &pi));
	}

	{
		VK_CHECK(vkDeviceWaitIdle(vulkanDevice.GetDevice()));
	}

	return true;
}

void AppBase::UpdateCommandBuffer(const std::vector<RendererBase*>& renderers, uint32_t imageIndex)
{
	VkCommandBuffer commandBuffer = vulkanDevice.GetCommandBuffer(imageIndex);

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

	VK_CHECK(vkEndCommandBuffer(commandBuffer));
}

void AppBase::InitIMGUI()
{
	showImgui = true;

	/*IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();*/

	// Setup Platform/Renderer backends
}

void AppBase::InitCamera()
{
	camera = std::make_unique<Camera>(glm::vec3(0.0f, 0.0f, 3.0f));
	lastX = static_cast<float>(AppSettings::ScreenWidth) / 2.0f;
	lastY = static_cast<float>(AppSettings::ScreenHeight) / 2.0f;
	firstMouse = true;
}

void AppBase::InitTiming()
{
	deltaTime = 0.0f;	// Time between current frame and last frame
	lastFrame = 0.0f;
}

void AppBase::ProcessTiming()
{
	// Per-frame time
	float currentFrame = static_cast<float>(glfwGetTime());
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
}

int AppBase::GLFWWindowShouldClose()
{
	return glfwWindowShouldClose(glfwWindow);
}

void AppBase::PollEvents()
{
	glfwPollEvents();
}

void AppBase::Terminate()
{
	glfwDestroyWindow(glfwWindow);
	glfwTerminate();

	vulkanDevice.Destroy();
	vulkanInstance.Destroy();
}

void AppBase::FrameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
	// TODO
}

void AppBase::MouseCallback(GLFWwindow* window, double xposIn, double yposIn)
{
	if (!middleMousePressed)
	{
		return;
	}

	float xPos = static_cast<float>(xposIn);
	float yPos = static_cast<float>(yposIn);
	if (firstMouse)
	{
		lastX = xPos;
		lastY = yPos;
		firstMouse = false;
	}
	float xOffset = xPos - lastX;
	float yOffset = lastY - yPos; // reversed since y-coordinates go from bottom to top
	lastX = xPos;
	lastY = yPos;
	camera->ProcessMouseMovement(xOffset, yOffset);
}

void AppBase::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
	{
		middleMousePressed = true;
	}
	else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
	{
		middleMousePressed = false;
		firstMouse = true;
	}
}

void AppBase::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera->ProcessMouseScroll(static_cast<float>(yoffset));
}

void AppBase::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_I && action == GLFW_PRESS)
	{
		// Toggle imgui window
		showImgui = !showImgui;
	}
}

// Process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void AppBase::ProcessInput()
{
	if (glfwGetKey(glfwWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(glfwWindow, true);
	}

	if (glfwGetKey(glfwWindow, GLFW_KEY_W) == GLFW_PRESS)
	{
		camera->ProcessKeyboard(CameraForward, deltaTime);
	}

	if (glfwGetKey(glfwWindow, GLFW_KEY_S) == GLFW_PRESS)
	{
		camera->ProcessKeyboard(CameraBackward, deltaTime);
	}

	if (glfwGetKey(glfwWindow, GLFW_KEY_A) == GLFW_PRESS)
	{
		camera->ProcessKeyboard(CameraLeft, deltaTime);
	}

	if (glfwGetKey(glfwWindow, GLFW_KEY_D) == GLFW_PRESS)
	{
		camera->ProcessKeyboard(CameraRight, deltaTime);
	}
}