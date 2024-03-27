#ifndef APP_SKINNING
#define APP_SKINNING

#include "AppBase.h"
#include "Scene.h"
#include "ResourcesLight.h"
#include "PipelineImGui.h"
#include "PipelinePBRBindless.h"
#include "PipelineLightRender.h"

#include <memory>

class AppSkinning final : AppBase
{
public:
	AppSkinning();
	void MainLoop() override;
	void UpdateUBOs() override;
	void UpdateUI() override;

	void Init();
	void InitLights();

private:
	PipelinePBRBindless* pbrPtr_ = nullptr;
	PipelineLightRender* lightPtr_ = nullptr;
	PipelineImGui* imguiPtr_ = nullptr;

	std::unique_ptr<Scene> scene_ = nullptr;
	ResourcesLight* resourcesLight_ = nullptr;
};

#endif