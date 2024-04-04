#ifndef APP_SIMPLE_RAYTRACING
#define APP_SIMPLE_RAYTRACING

#include "AppBase.h"
#include "Scene.h"
#include "PipelineImGui.h"
#include "PipelineSimpleRaytracing.h"
#include "ResourcesLight.h"

// STL
#include <memory>

class Scene;

class AppSimpleRaytracing final : AppBase
{
public:
	AppSimpleRaytracing();
	
	void MainLoop() override;
	void UpdateUBOs() override;
	void UpdateUI() override;

	void Init();
	void InitLights();

private:
	PipelineImGui* imguiPtr_ = nullptr;
	PipelineSimpleRaytracing* rtxPtr_ = nullptr;
	std::unique_ptr<Scene> scene_ = nullptr;
	ResourcesLight* resourcesLight_ = nullptr;
};

#endif