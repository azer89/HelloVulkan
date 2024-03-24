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
	void InitImGui();
	void InitGLSLang();
	void InitGLFW();
	void InitCamera();

	// Resources
	void InitSharedResources(); // TODO Delete this
	void InitSharedResources2();
	
	// Functions related to the main loop
	bool StillRunning();
	void PollEvents();
	void ProcessTiming();
	void ProcessInput();

	// Should be used to destroy resources
	void DestroyInternal();
	
	template<class T, class... U>
	T* AddPipeline(U&&... u)
	{
		std::unique_ptr<T> pipeline = std::make_unique<T>(std::forward<U>(u)...);
		T* ptr = pipeline.get();
		pipelines2_.push_back(std::move(pipeline));
		return ptr;
	}

	template<class T, class... U>
	T* AddResources(U&&... u)
	{
		std::unique_ptr<T> resources = std::make_unique<T>(std::forward<U>(u)...);
		T* ptr = resources.get();
		resources_.push_back(std::move(resources));
		return ptr;
	}

protected:
	GLFWwindow* glfwWindow_;

	// Camera
	std::unique_ptr<Camera> camera_;
	float lastX_;
	float lastY_;
	bool firstMouse_;
	bool leftMousePressed_;
	bool showImgui_;

	// Timing
	FrameCounter frameCounter_;

	// These two are not copyable or movable
	VulkanInstance vulkanInstance_;
	VulkanContext vulkanContext_;

	// A list of pipelines (graphics and compute)
	std::vector<PipelineBase*> pipelines_; // TODO Delete this
	std::vector<std::unique_ptr<PipelineBase>> pipelines2_; // TODO Rename to pipelines_
	std::vector<std::unique_ptr<ResourcesBase>> resources_;

	// Window size
	uint32_t windowWidth_;
	uint32_t windowHeight_;
	bool shouldRecreateSwapchain_;

	std::unique_ptr<ResourcesShared> resShared_; // TODO Delete
	std::unique_ptr<ResourcesIBL> resIBL_; // TODO Delete
	ResourcesShared* resShared2_;
	ResourcesIBL* resIBL2_;

	InputContext inputContext_ = {};
};

#endif