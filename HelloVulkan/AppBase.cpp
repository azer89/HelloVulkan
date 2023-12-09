#include "AppBase.h"
#include "AppSettings.h"

#define VK_NO_PROTOTYPES
#include "volk.h"

#include <iostream>

AppBase::AppBase()
{
	InitGLFW();
	InitVulkan();
	InitIMGUI();
	InitCamera();
	InitTiming();
}

void AppBase::InitVulkan()
{
	VkResult res = volkInitialize();
	if (res != VK_SUCCESS)
	{
		std::cerr << "Volk Cannot be initialized\n";
	}
	vulkanInstance.Create();
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
	{
		auto func = [](GLFWwindow* window, int width, int height)
			{
				static_cast<AppBase*>(glfwGetWindowUserPointer(window))->FrameBufferSizeCallback(window, width, height);
			};
		glfwSetFramebufferSizeCallback(glfwWindow, func);
	}
	{
		auto func = [](GLFWwindow* window, double xpos, double ypos)
			{
				static_cast<AppBase*>(glfwGetWindowUserPointer(window))->MouseCallback(window, xpos, ypos);
			};
		glfwSetCursorPosCallback(glfwWindow, func);
	}
	{
		auto func = [](GLFWwindow* window, int button, int action, int mods)
			{
				static_cast<AppBase*>(glfwGetWindowUserPointer(window))->MouseButtonCallback(window, button, action, mods);
			};
		glfwSetMouseButtonCallback(glfwWindow, func);
	}
	{
		auto func = [](GLFWwindow* window, double xoffset, double yoffset)
			{
				static_cast<AppBase*>(glfwGetWindowUserPointer(window))->ScrollCallback(window, xoffset, yoffset);
			};
		glfwSetScrollCallback(glfwWindow, func);
	}
	{
		auto func = [](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				static_cast<AppBase*>(glfwGetWindowUserPointer(window))->KeyCallback(window, key, scancode, action, mods);
			};
		glfwSetKeyCallback(glfwWindow, func);
	}
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

	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}
	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
	lastX = xpos;
	lastY = ypos;
	camera->ProcessMouseMovement(xoffset, yoffset);
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