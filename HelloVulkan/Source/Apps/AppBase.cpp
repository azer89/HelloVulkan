#include "AppBase.h"
#include "Configs.h"
#include "ResourcesShared.h"
#include "VulkanUtility.h"

#include "volk.h"

// Init GLSLang
#include "glslang_c_interface.h" 

#include "imgui_impl_vulkan.h"

#include <iostream>
#include <array>

AppBase::AppBase() :
	shouldRecreateSwapchain_(false)
{
	InitGLFW();
	InitGLSLang();
	InitImGui();
	InitCamera();
}

void AppBase::InitVulkan(ContextConfig config)
{
	// Initialize Volk
	const VkResult res = volkInitialize();
	if (res != VK_SUCCESS)
	{
		std::cerr << "Volk Cannot be initialized\n";
	}

	// Initialize Vulkan instance
	vulkanInstance_.Create();
	vulkanInstance_.SetupDebugCallbacks();
	vulkanInstance_.CreateWindowSurface(glfwWindow_);
	vulkanContext_.Create(vulkanInstance_, config);
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

	constexpr int w = AppConfig::InitialScreenWidth;
	constexpr int h = AppConfig::InitialScreenHeight;

	// GLFW window creation
	windowWidth_ = static_cast<uint32_t>(w);
	windowHeight_ = static_cast<uint32_t>(h);
	glfwWindow_ = glfwCreateWindow(
		w,
		h,
		AppConfig::ScreenTitle.c_str(),
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

void AppBase::DrawFrame()
{
	FrameData& frameData = vulkanContext_.GetCurrentFrameData();

	vkWaitForFences(vulkanContext_.GetDevice(), 1, &(frameData.queueSubmitFence_), VK_TRUE, UINT64_MAX);

	VkResult result = vulkanContext_.GetNextSwapchainImage(frameData.nextSwapchainImageSemaphore_);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		OnWindowResized();
		return;
	}
	uint32_t swapchainImageIndex = vulkanContext_.GetCurrentSwapchainImageIndex();

	vkResetFences(vulkanContext_.GetDevice(), 1, &(frameData.queueSubmitFence_));
	vkResetCommandBuffer(frameData.graphicsCommandBuffer_, 0);

	// ImGui first then UBOs, because ImGui sets a few UBO values
	UpdateUI();

	// Send UBOs to buffers
	UpdateUBOs();

	// Start recording command buffers
	FillCommandBuffer(frameData.graphicsCommandBuffer_);

	constexpr VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	std::array<VkSemaphore, 1> waitSemaphores{ frameData.nextSwapchainImageSemaphore_ };

	// Submit Queue
	// TODO Set code below as a function in VulkanDevice
	const VkSubmitInfo submitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
		.pWaitSemaphores = waitSemaphores.data(),
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1u,
		.pCommandBuffers = &(frameData.graphicsCommandBuffer_),
		.signalSemaphoreCount = 1u,
		.pSignalSemaphores = &(frameData.graphicsQueueSemaphore_)
	};
	VK_CHECK(vkQueueSubmit(vulkanContext_.GetGraphicsQueue(), 1, &submitInfo, frameData.queueSubmitFence_));

	// Present
	// TODO Set code below as a function in VulkanDevice
	const VkPresentInfoKHR presentInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1u,
		.pWaitSemaphores = &(frameData.graphicsQueueSemaphore_),
		.swapchainCount = 1u,
		.pSwapchains = vulkanContext_.GetSwapchainPtr(),
		.pImageIndices = &swapchainImageIndex
	};
	result = vkQueuePresentKHR(vulkanContext_.GetGraphicsQueue(), &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || shouldRecreateSwapchain_)
	{
		OnWindowResized();
	}

	// Do this after the end of the draw
	vulkanContext_.IncrementFrameIndex();

	FrameMark;
}

void AppBase::UpdateUI()
{
	// Empty, must be implemented in a derived class
}

// Fill/record command buffer
void AppBase::FillCommandBuffer(VkCommandBuffer commandBuffer)
{
	constexpr VkCommandBufferBeginInfo beginInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		.pInheritanceInfo = nullptr
	};

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	// Iterate through all pipelines to fill the command buffer
	for (const auto& pip : pipelines_)
	{
		pip->FillCommandBuffer(vulkanContext_, commandBuffer);
	}

	// Tracy
	TracyVkCollect(vulkanContext_.GetTracyContext(), commandBuffer);

	VK_CHECK(vkEndCommandBuffer(commandBuffer));
}

// Recreate resources when window is resized
void AppBase::OnWindowResized()
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(glfwWindow_, &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(glfwWindow_, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(vulkanContext_.GetDevice());

	vulkanContext_.RecreateSwapchainResources(
		vulkanInstance_,
		windowWidth_,
		windowHeight_
	);

	InitSharedResources();

	for (const auto& pip : pipelines_)
	{
		pip->OnWindowResized(vulkanContext_);
	}

	shouldRecreateSwapchain_ = false;
}

void AppBase::InitImGui()
{
	showImgui_ = true;
}

void AppBase::InitCamera()
{
	camera_ = std::make_unique<Camera>(glm::vec3(0.0f, 0.0f, 3.0f));
	lastX_ = static_cast<float>(windowWidth_) / 2.0f;
	lastY_ = static_cast<float>(windowHeight_) / 2.0f;
	firstMouse_ = true;
}

void AppBase::ProcessTiming()
{
	// Per-frame time
	float currentFrame = static_cast<float>(glfwGetTime());
	frameCounter_.Update(currentFrame);
}

bool AppBase::StillRunning()
{
	if (glfwWindowShouldClose(glfwWindow_))
	{
		vkDeviceWaitIdle(vulkanContext_.GetDevice());
		return false;
	}
	return true;
}

void AppBase::PollEvents()
{
	glfwPollEvents();
}

void AppBase::DestroyInternal()
{
	glfwDestroyWindow(glfwWindow_);
	glfwTerminate();

	resShared_.reset();

	vulkanContext_.Destroy();
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

	shouldRecreateSwapchain_ = true;
}

void AppBase::MouseCallback(GLFWwindow* window, double xposIn, double yposIn)
{
	if (!leftMousePressed_)
	{
		return;
	}

	const float xPos = static_cast<float>(xposIn);
	const float yPos = static_cast<float>(yposIn);
	if (firstMouse_)
	{
		lastX_ = xPos;
		lastY_ = yPos;
		firstMouse_ = false;
	}
	const float xOffset = xPos - lastX_;
	const float yOffset = lastY_ - yPos; // reversed since y-coordinates go from bottom to top
	lastX_ = xPos;
	lastY_ = yPos;
	camera_->ProcessMouseMovement(xOffset, yOffset);
}

void AppBase::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (const auto& io = ImGui::GetIO(); io.WantCaptureMouse)
	{
		return;
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		leftMousePressed_ = true;
	}
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		leftMousePressed_ = false;
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
		camera_->ProcessKeyboard(CameraMovement::Forward, frameCounter_.GetDeltaSecond());
	}

	if (glfwGetKey(glfwWindow_, GLFW_KEY_S) == GLFW_PRESS)
	{
		camera_->ProcessKeyboard(CameraMovement::Backward, frameCounter_.GetDeltaSecond());
	}

	if (glfwGetKey(glfwWindow_, GLFW_KEY_A) == GLFW_PRESS)
	{
		camera_->ProcessKeyboard(CameraMovement::Left, frameCounter_.GetDeltaSecond());
	}

	if (glfwGetKey(glfwWindow_, GLFW_KEY_D) == GLFW_PRESS)
	{
		camera_->ProcessKeyboard(CameraMovement::Right, frameCounter_.GetDeltaSecond());
	}
}

void AppBase::InitSharedResources()
{
	if (!resShared_)
	{
		resShared_ = std::make_unique<ResourcesShared>();
	}
	resShared_->Create(vulkanContext_);
}