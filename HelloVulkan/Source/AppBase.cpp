#include "AppBase.h"
#include "Configs.h"
#include "VulkanUtility.h"

#include "volk.h"

// Init GLSLang
#include "glslang_c_interface.h" 

#include "imgui_impl_glfw.h"
#include "imgui_impl_volk.h"

#include <iostream>
#include <array>

AppBase::AppBase() :
	shouldRecreateSwapchain_(false)
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

	int w = AppConfig::InitialScreenWidth;
	int h = AppConfig::InitialScreenHeight;

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

void AppBase::CreateSharedImageResources()
{
	depthImage_.Destroy(vulkanDevice_.GetDevice());
	multiSampledColorImage_.Destroy(vulkanDevice_.GetDevice());
	singleSampledColorImage_.Destroy(vulkanDevice_.GetDevice());

	const VkSampleCountFlagBits msaaSamples = vulkanDevice_.GetMSAASampleCount();
	const uint32_t width = vulkanDevice_.GetFrameBufferWidth();
	const uint32_t height = vulkanDevice_.GetFrameBufferHeight();

	// Depth attachment (OnScreen and offscreen)
	depthImage_.CreateDepthResources(
		vulkanDevice_,
		width,
		height,
		msaaSamples);
	depthImage_.SetDebugName(vulkanDevice_, "Depth_Image");

	// Color attachments
	// Multi-sampled (MSAA)
	multiSampledColorImage_.CreateColorResources(
		vulkanDevice_,
		width,
		height,
		msaaSamples);
	multiSampledColorImage_.SetDebugName(vulkanDevice_, "Multisampled_Color_Image");

	// Single-sampled
	singleSampledColorImage_.CreateColorResources(
		vulkanDevice_,
		width,
		height);
	singleSampledColorImage_.SetDebugName(vulkanDevice_, "Singlesampled_Color_Image");
}

bool AppBase::DrawFrame()
{
	FrameData& frameData = vulkanDevice_.GetCurrentFrameData();

	vkWaitForFences(vulkanDevice_.GetDevice(), 1, &(frameData.queueSubmitFence_), VK_TRUE, UINT64_MAX);

	uint32_t imageIndex = 0;
	VkResult result = vkAcquireNextImageKHR(
		vulkanDevice_.GetDevice(), 
		vulkanDevice_.GetSwapChain(), 
		0, 
		// Wait for the swapchain image to become available
		frameData.nextSwapchainImageSemaphore_,
		VK_NULL_HANDLE, 
		&imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		OnWindowResized();
		return true;
	}

	vkResetFences(vulkanDevice_.GetDevice(), 1, &(frameData.queueSubmitFence_));
	vkResetCommandBuffer(frameData.commandBuffer_, 0);
	vkResetCommandBuffer(frameData.compCommandBuffer_, 0);

	// Send UBOs to shaders
	UpdateUBOs(imageIndex);

	// ImGui
	UpdateUI();

	// Compute
	{
		vkResetCommandBuffer(frameData.compCommandBuffer_, 0);

		VkCommandBufferBeginInfo commandBufferBeginInfo = 
		{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		};

		VK_CHECK(vkBeginCommandBuffer(frameData.compCommandBuffer_, &commandBufferBeginInfo));

		FillComputeCommandBuffer(frameData.compCommandBuffer_, imageIndex);

		VK_CHECK(vkEndCommandBuffer(frameData.compCommandBuffer_));

		VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
		VkSubmitInfo computeSubmitInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pNext = 0,
			.waitSemaphoreCount = 0,
			.pWaitSemaphores = nullptr,
			.pWaitDstStageMask = &waitStageMask,
			.commandBufferCount = 1,
			.pCommandBuffers = &(frameData.compCommandBuffer_),
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &(frameData.computeSemaphore_)
		};
		
		VK_CHECK(vkQueueSubmit(vulkanDevice_.GetComputeQueue(), 1, &computeSubmitInfo, VK_NULL_HANDLE));
	}

	// Start recording command buffers
	FillCommandBuffer(frameData.commandBuffer_, imageIndex);

	//const VkPipelineStageFlags waitStages[] = 
	//	{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkPipelineStageFlags graphicsWaitStageMasks[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	std::array<VkSemaphore, 2> waitSemaphores = { frameData.computeSemaphore_, frameData.nextSwapchainImageSemaphore_ };

	const VkSubmitInfo submitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = nullptr,
		.waitSemaphoreCount = 2u,
		// Wait for the swapchain image to become available
		//.pWaitSemaphores = &(frameData.nextSwapchainImageSemaphore_),
		.pWaitSemaphores = waitSemaphores.data(),
		.pWaitDstStageMask = graphicsWaitStageMasks,
		.commandBufferCount = 1u,
		.pCommandBuffers = &(frameData.commandBuffer_),
		.signalSemaphoreCount = 1u,
		// Wait for rendering to complete
		.pSignalSemaphores = &(frameData.renderSemaphore_)
	};

	VK_CHECK(vkQueueSubmit(vulkanDevice_.GetGraphicsQueue(), 1, &submitInfo, frameData.queueSubmitFence_));

	const VkPresentInfoKHR presentInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1u,
		// Wait for rendering to complete
		.pWaitSemaphores = &(frameData.renderSemaphore_),
		.swapchainCount = 1u,
		.pSwapchains = vulkanDevice_.GetSwapchainPtr(),
		.pImageIndices = &imageIndex
	};

	result = vkQueuePresentKHR(vulkanDevice_.GetGraphicsQueue(), &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || shouldRecreateSwapchain_)
	{
		OnWindowResized();
	}

	// Do this after the end of the draw
	vulkanDevice_.IncrementFrameIndex();

	return true;
}

void AppBase::UpdateUI()
{
	// Empty, must be implemented in a derived class
}

void AppBase::FillComputeCommandBuffer(VkCommandBuffer compCommandBuffer, uint32_t imageIndex)
{
	// Empty, must be implemented in a derived class
}

// Fill/record command buffer
void AppBase::FillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	const VkCommandBufferBeginInfo beginIndo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		.pInheritanceInfo = nullptr
	};

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginIndo));

	// Iterate through all renderers to fill the command buffer
	for (auto& r : graphicsPipelines_)
	{
		r->FillCommandBuffer(vulkanDevice_, commandBuffer, imageIndex);
	}

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

	vkDeviceWaitIdle(vulkanDevice_.GetDevice());

	vulkanDevice_.RecreateSwapchainResources(
		vulkanInstance_,
		windowWidth_,
		windowHeight_
	);

	CreateSharedImageResources();

	for (auto& r : graphicsPipelines_)
	{
		r->OnWindowResized(vulkanDevice_);
	}

	for (auto& r : computePipelines_)
	{
		r->OnWindowResized(vulkanDevice_);
	}

	shouldRecreateSwapchain_ = false;
}

void AppBase::InitIMGUI()
{
	showImgui_ = true;
}

void AppBase::InitCamera()
{
	camera_ = std::make_unique<Camera>(glm::vec3(0.0f, 6.0f, 18.0f));
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

	depthImage_.Destroy(vulkanDevice_.GetDevice());
	multiSampledColorImage_.Destroy(vulkanDevice_.GetDevice());
	singleSampledColorImage_.Destroy(vulkanDevice_.GetDevice());

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

	shouldRecreateSwapchain_ = true;
}

void AppBase::MouseCallback(GLFWwindow* window, double xposIn, double yposIn)
{
	if (!leftMousePressed_)
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