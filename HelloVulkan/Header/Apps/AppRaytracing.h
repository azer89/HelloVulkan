#ifndef APP_RAYTRACING
#define APP_RAYTRACING

#include "AppBase.h"
#include "Scene.h"
#include "PipelineImGui.h"
#include "PipelineRaytracing.h"
#include "ResourcesLight.h"

// STL
#include <memory>

class Scene;

class AppRaytracing final : AppBase
{
public:
	AppRaytracing();
	
	void MainLoop() override;
	void UpdateUBOs() override;
	void UpdateUI() override;

	void Init();
	void InitLights();

private:
	PipelineImGui* imguiPtr_ = nullptr;
	PipelineRaytracing* rtxPtr_ = nullptr;
	std::unique_ptr<Scene> scene_ = nullptr;
	ResourcesLight* resourcesLight_ = nullptr;
};

#endif