#ifndef APP_SIMPLE_RAYTRACING
#define APP_SIMPLE_RAYTRACING

#include "AppBase.h"
#include "Scene.h"
#include "PipelineClear.h"
#include "PipelineImGui.h"
#include "PipelineFinish.h"
#include "PipelineSimpleRaytracing.h"

// STL
#include <memory>

class Scene;

class AppSimpleRaytracing final : AppBase
{
public:
	AppSimpleRaytracing();
	void Init();
	void MainLoop() override;
	void UpdateUBOs() override;
	void UpdateUI() override;

private:
	PipelineImGui* imguiPtr_ = nullptr;
	PipelineSimpleRaytracing* rtxPtr_ = nullptr;
	std::unique_ptr<Scene> scene_ = nullptr;
};

#endif