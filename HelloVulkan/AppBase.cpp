#include "AppBase.h"
#include "AppSettings.h"
#include "VulkanUtility.h"

#include "volk.h"
#include "glslang_c_interface.h" // Init GLSLang

#include <iostream>

AppBase::AppBase() :
	recreateSwapchain_(false)
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

	// Multisampling / MSAA
	VkPhysicalDeviceFeatures features = {};
	features.sampleRateShading = VK_TRUE;
	features.samplerAnisotropy = VK_TRUE;

	// Initialize Vulkan instance
	vulkanInstance_.Create();
	vulkanInstance_.SetupDebugCallbacks();
	vulkanInstance_.CreateWindowSurface(glfwWindow_);
	vulkanDevice_.CreateCompute(vulkanInstance_,
		static_cast<uint32_t>(windowWidth_),
		static_cast<uint32_t>(windowHeight_),
		features);
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

	int w = AppSettings::InitialScreenWidth;
	int h = AppSettings::InitialScreenHeight;

	// GLFW window creation
	windowWidth_ = static_cast<uint32_t>(w);
	windowHeight_ = static_cast<uint32_t>(h);
	glfwWindow_ = glfwCreateWindow(
		w,
		h,
		AppSettings::ScreenTitle.c_str(),
		nullptr,
		nullptr);
	if (glfwWindow_ == nullptr)
	{
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window");
	}
	glfwMakeContextCurrent(glfwWindow_);

	glfwSetWindowUserPointer(glfwWindow_, this);
	
	auto FuncFramebuffer = [](GLFWwindow* window, int width, int height)
	{
		static_cast<AppBase*>(glfwGetWindowUserPointer(window))->FrameBufferSizeCallback(window, width, height);
	};
	glfwSetFramebufferSizeCallback(glfwWindow_, FuncFramebuffer);

	auto FuncCursor = [](GLFWwindow* window, double x, double y)
	{
		static_cast<AppBase*>(glfwGetWindowUserPointer(window))->MouseCallback(window, x, y);
	};
	glfwSetCursorPosCallback(glfwWindow_, FuncCursor);

	auto FuncMouse = [](GLFWwindow* window, int button, int action, int mods)
	{
		static_cast<AppBase*>(glfwGetWindowUserPointer(window))->MouseButtonCallback(window, button, action, mods);
	};
	glfwSetMouseButtonCallback(glfwWindow_, FuncMouse);

	auto FuncScroll = [](GLFWwindow* window, double xOffset, double yOffset)
	{
		static_cast<AppBase*>(glfwGetWindowUserPointer(window))->ScrollCallback(window, xOffset, yOffset);
	};
	glfwSetScrollCallback(glfwWindow_, FuncScroll);

	auto FuncKey = [](GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		static_cast<AppBase*>(glfwGetWindowUserPointer(window))->KeyCallback(window, key, scancode, action, mods);
	};
	glfwSetKeyCallback(glfwWindow_, FuncKey);
}

bool AppBase::DrawFrame()
{
	uint32_t imageIndex = 0;
	VkResult result = vkAcquireNextImageKHR(
		vulkanDevice_.GetDevice(), 
		vulkanDevice_.GetSwapChain(), 
		0, 
		// Wait for the swapchain image to become available
		*(vulkanDevice_.GetSwapchainSemaphorePtr()), 
		VK_NULL_HANDLE, 
		&imageIndex);

	// Need to recreate swapchain images and framebuffers 
	// because the window is resized 
	if (recreateSwapchain_ || result != VK_SUCCESS)
	{
		OnWindowResized();
		recreateSwapchain_ = false;
	}

	VK_CHECK(vkResetCommandPool(vulkanDevice_.GetDevice(), vulkanDevice_.GetCommandPool(), 0));

	// Send UBOs to shaders
	UpdateUBOs(imageIndex);

	// Rendering here
	FillCommandBuffer(imageIndex);

	const VkPipelineStageFlags waitStages[] = 
		{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	const VkSubmitInfo submitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 1u,
		// Wait for the swapchain image to become available
		.pWaitSemaphores = vulkanDevice_.GetSwapchainSemaphorePtr(),
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1u,
		.pCommandBuffers = vulkanDevice_.GetCommandBufferPtr(imageIndex),
		.signalSemaphoreCount = 1u,
		// Wait for rendering to complete
		.pSignalSemaphores = vulkanDevice_.GetRenderSemaphorePtr()
	};

	VK_CHECK(vkQueueSubmit(vulkanDevice_.GetGraphicsQueue(), 1, &submitInfo, nullptr));

	const VkPresentInfoKHR presentInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1u,
		// Wait for rendering to complete
		.pWaitSemaphores = vulkanDevice_.GetRenderSemaphorePtr(),
		.swapchainCount = 1u,
		.pSwapchains = vulkanDevice_.GetSwapchainPtr(),
		.pImageIndices = &imageIndex
	};

	result = vkQueuePresentKHR(vulkanDevice_.GetGraphicsQueue(), &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapchain_ = true;
	}

	VK_CHECK(vkDeviceWaitIdle(vulkanDevice_.GetDevice()));

	return true;
}

void AppBase::FillCommandBuffer(uint32_t imageIndex)
{
	VkCommandBuffer commandBuffer = vulkanDevice_.GetCommandBuffer(imageIndex);

	const VkCommandBufferBeginInfo beginIndo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		.pInheritanceInfo = nullptr
	};

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginIndo));

	// Iterate through all renderers to fill the command buffer
	for (auto& r : renderers_)
	{
		r->FillCommandBuffer(vulkanDevice_, commandBuffer, imageIndex);
	}

	VK_CHECK(vkEndCommandBuffer(commandBuffer));
}

void AppBase::OnWindowResized()
{
	for (auto& r : renderers_)
	{
		r->DestroySwapchainFrameBufferOnWindowResized(vulkanDevice_);
	}

	vulkanDevice_.RecreateSwapchainResources(
		vulkanInstance_,
		windowWidth_,
		windowHeight_
	);

	for (auto& r : renderers_)
	{
		r->RecreateSwapchainFramebufferOnWindowResized(vulkanDevice_);
	}
}

void AppBase::InitIMGUI()
{
	showImgui_ = true;

	/*IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();*/

	// Setup Platform/Renderer backends
}

void AppBase::InitCamera()
{
	camera_ = std::make_unique<Camera>(glm::vec3(0.0f, 0.0f, 3.0f));
	lastX_ = static_cast<float>(windowWidth_) / 2.0f;
	lastY_ = static_cast<float>(windowHeight_) / 2.0f;
	firstMouse_ = true;
}

void AppBase::InitTiming()
{
	deltaTime_ = 0.0f;	// Time between current frame and last frame
	lastFrame_ = 0.0f;
}

void AppBase::ProcessTiming()
{
	// Per-frame time
	float currentFrame = static_cast<float>(glfwGetTime());
	deltaTime_ = currentFrame - lastFrame_;
	lastFrame_ = currentFrame;
}

int AppBase::GLFWWindowShouldClose()
{
	return glfwWindowShouldClose(glfwWindow_);
}

void AppBase::PollEvents()
{
	glfwPollEvents();
}

void AppBase::Terminate()
{
	glfwDestroyWindow(glfwWindow_);
	glfwTerminate();

	vulkanDevice_.Destroy();
	vulkanInstance_.Destroy();
}

void AppBase::FrameBufferSizeCallback(GLFWwindow* window, int width, int height)
{
	windowWidth_ = static_cast<uint32_t>(width);
	windowHeight_ = static_cast<uint32_t>(height);

	camera_->SetScreenSize(
		static_cast<float>(windowWidth_),
		static_cast<float>(windowHeight_)
	);
}

void AppBase::MouseCallback(GLFWwindow* window, double xposIn, double yposIn)
{
	if (!middleMousePressed_)
	{
		return;
	}

	float xPos = static_cast<float>(xposIn);
	float yPos = static_cast<float>(yposIn);
	if (firstMouse_)
	{
		lastX_ = xPos;
		lastY_ = yPos;
		firstMouse_ = false;
	}
	float xOffset = xPos - lastX_;
	float yOffset = lastY_ - yPos; // reversed since y-coordinates go from bottom to top
	lastX_ = xPos;
	lastY_ = yPos;
	camera_->ProcessMouseMovement(xOffset, yOffset);
}

void AppBase::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
	{
		middleMousePressed_ = true;
	}
	else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
	{
		middleMousePressed_ = false;
		firstMouse_ = true;
	}
}

void AppBase::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera_->ProcessMouseScroll(static_cast<float>(yoffset));
}

void AppBase::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_I && action == GLFW_PRESS)
	{
		// Toggle imgui window
		showImgui_ = !showImgui_;
	}
}

// Process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void AppBase::ProcessInput()
{
	if (glfwGetKey(glfwWindow_, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(glfwWindow_, true);
	}

	if (glfwGetKey(glfwWindow_, GLFW_KEY_W) == GLFW_PRESS)
	{
		camera_->ProcessKeyboard(CameraForward, deltaTime_);
	}

	if (glfwGetKey(glfwWindow_, GLFW_KEY_S) == GLFW_PRESS)
	{
		camera_->ProcessKeyboard(CameraBackward, deltaTime_);
	}

	if (glfwGetKey(glfwWindow_, GLFW_KEY_A) == GLFW_PRESS)
	{
		camera_->ProcessKeyboard(CameraLeft, deltaTime_);
	}

	if (glfwGetKey(glfwWindow_, GLFW_KEY_D) == GLFW_PRESS)
	{
		camera_->ProcessKeyboard(CameraRight, deltaTime_);
	}
}