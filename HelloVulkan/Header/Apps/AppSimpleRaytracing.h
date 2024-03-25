#ifndef APP_SIMPLE_RAYTRACING
#define APP_SIMPLE_RAYTRACING

#include "AppBase.h"
#include "Scene.h"
#include "PipelineClear.h"
#include "PipelineFinish.h"
#include "PipelineImGui.h"
#include "PipelineSimpleRaytracing.h"

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

private:
	PipelineImGui* imguiPtr_;
	PipelineSimpleRaytracing* rtxPtr_;
	std::unique_ptr<Scene> scene_;
};

#endif