#ifndef APP_BASE
#define APP_BASE

#include "VulkanInstance.h"
#include "VulkanContext.h"
#include "PipelineBase.h"
#include "FrameCounter.h"
#include "Camera.h"
#include "ResourcesShared.h"
#include "ResourcesIBL.h"
#include "InputContext.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include <memory>

class AppBase
{
public:
	AppBase();
	virtual void MainLoop() = 0; 

protected:
	virtual void UpdateUI();
	virtual void UpdateUBOs() = 0;
	void FillCommandBuffer(VkCommandBuffer commandBuffer);
	
	void OnWindowResized();
	void DrawFrame();

	// GLFW callbacks
	void FrameBufferSizeCallback(GLFWwindow* window, int width, int height);
	void MouseCallback(GLFWwindow* window, double xpos, double ypos);
	void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
	void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

	// Init functions
	void InitVulkan(ContextConfig config);
	void InitGLSLang();
	void InitGLFW();
	void InitCamera();
	
	// Functions related to the main loop
	bool StillRunning();
	void PollEvents();
	void ProcessTiming();
	void ProcessInput();

	// ImGui
	bool ShowImGui();

	void DestroyResources();

	// Resources
	void InitSharedResources();

	template<class T, class... U>
	T* AddPipeline(U&&... u)
	{
		// Create std::unique_ptr of pipeline
		std::unique_ptr<T> pipeline = std::make_unique<T>(std::forward<U>(u)...);
		T* ptr = pipeline.get();
		pipelines_.push_back(std::move(pipeline)); // Put it in std::vector
		return ptr;
	}

	template<class T, class... U>
	T* AddResources(U&&... u)
	{
		// Create std::unique_ptr of resources
		std::unique_ptr<T> resources = std::make_unique<T>(std::forward<U>(u)...);
		T* ptr = resources.get();
		resources_.push_back(std::move(resources)); // Put it in std::vector
		return ptr;
	}

protected:
	GLFWwindow* glfwWindow_ = nullptr;

	// Camera
	std::unique_ptr<Camera> camera_ = nullptr;
	/*float lastX_;
	float lastY_;
	bool firstMouse_;
	bool leftMousePressed_;
	bool showImgui_;*/

	// Timing
	FrameCounter frameCounter_;

	// These two are not copyable or movable
	VulkanInstance vulkanInstance_;
	VulkanContext vulkanContext_;

	// A list of pipelines (graphics, compute, or raytracing)
	std::vector<std::unique_ptr<PipelineBase>> pipelines_ = {};

	// A list of resources containing buffers and images
	std::vector<std::unique_ptr<ResourcesBase>> resources_ = {};

	// Window size
	uint32_t windowWidth_;
	uint32_t windowHeight_;
	bool shouldRecreateSwapchain_;

	ResourcesShared* resourcesShared_ = nullptr;
	ResourcesIBL* resourcesIBL_ = nullptr;

	InputContext inputContext_ = {};
};

#endif