#ifndef APP_BASE
#define APP_BASE

#include "VulkanInstance.h"
#include "VulkanUtility.h"
#include "VulkanDevice.h"
#include "Camera.h"
#include "RendererBase.h"

#include <memory>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

class AppBase
{
public:
	AppBase();
	virtual int MainLoop() = 0; 

private:
	GLFWwindow* glfwWindow;

protected:
	void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);
	void MouseCallback(GLFWwindow* window, double xpos, double ypos);
	void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
	void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

	void InitVulkan();
	void InitIMGUI();
	void InitGLSLang();
	void InitGLFW();
	void InitCamera();
	void InitTiming();
	
	int GLFWWindowShouldClose();
	void PollEvents();
	void ProcessTiming();
	void ProcessInput();
	void Terminate();

	bool DrawFrame(const std::vector<RendererBase*>& renderers);
	virtual void UpdateUBO(uint32_t imageIndex) = 0;
	void UpdateCommandBuffer(const std::vector<RendererBase*>& renderers, uint32_t imageIndex);

protected:
	// Camera
	std::unique_ptr<Camera> camera;
	float lastX;
	float lastY;
	bool firstMouse;
	bool middleMousePressed;
	bool showImgui;

	// Timing
	float deltaTime; // Time between current frame and last frame
	float lastFrame;

	// Vulkan
	VulkanInstance vulkanInstance;
	VulkanDevice vulkanDevice;
};

#endif