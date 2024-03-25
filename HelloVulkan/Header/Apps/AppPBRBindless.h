#ifndef APP_PBR_BINDLESS_TEXTURES
#define APP_PBR_BINDLESS_TEXTURES

#include "AppBase.h"
#include "Scene.h"
#include "ResourcesLight.h"
#include "PipelineSkybox.h"
#include "PipelineClear.h"
#include "PipelineFinish.h"
#include "PipelinePBRBindless.h"
#include "PipelineTonemap.h"
#include "PipelineResolveMS.h"
#include "PipelineLightRender.h"
#include "PipelineImGui.h"

// STL
#include <memory>

/*
Demo for bindless textures
*/
class AppPBRBindless final : AppBase
{
public:
	AppPBRBindless();
	void MainLoop() override;
	void UpdateUBOs() override;
	void UpdateUI() override;

	void Init();
	void InitLights();

private:
	PipelinePBRBindless* pbrPtr_;
	PipelineLightRender* lightPtr_;
	PipelineImGui* imguiPtr_;

	std::unique_ptr<Scene> scene_;
	ResourcesLight* resourcesLight_;
};

#endif