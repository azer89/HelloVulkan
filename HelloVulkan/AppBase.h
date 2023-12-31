#ifndef APP_BASE
#define APP_BASE

#include "VulkanInstance.h"
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

protected:
	virtual void UpdateUBOs(uint32_t imageIndex) = 0;
	void FillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void CreateSharedImageResources();
	void OnWindowResized();
	bool DrawFrame();

	// GLFW callbacks
	void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);
	void MouseCallback(GLFWwindow* window, double xpos, double ypos);
	void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
	void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

	// Init functions
	void InitVulkan();
	void InitIMGUI();
	void InitGLSLang();
	void InitGLFW();
	void InitCamera();
	void InitTiming();
	
	// Functions related to the main loop
	int GLFWWindowShouldClose();
	void PollEvents();
	void ProcessTiming();
	void ProcessInput();

	// Should be used to destroy resources
	void Terminate();

private:
	GLFWwindow* glfwWindow_;

protected:
	// Camera
	std::unique_ptr<Camera> camera_;
	float lastX_;
	float lastY_;
	bool firstMouse_;
	bool middleMousePressed_;
	bool showImgui_;

	// Timing
	float deltaTime_; // Time between current frame and last frame
	float lastFrame_;

	// Vulkan
	VulkanInstance vulkanInstance_;
	VulkanDevice vulkanDevice_;

	// A list of renderers
	std::vector<RendererBase*> renderers_;

	// Window size
	uint32_t windowWidth_;
	uint32_t windowHeight_;
	bool shouldRecreateSwapchain_;

	// Shared by multiple render passes
	// TODO change to unique ptrs
	VulkanImage multiSampledColorImage_;
	VulkanImage singleSampledColorImage_;
	VulkanImage depthImage_;
};

#endif