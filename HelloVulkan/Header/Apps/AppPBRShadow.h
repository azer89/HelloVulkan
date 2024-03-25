#ifndef APP_PBR_SHADOW_MAPPING
#define APP_PBR_SHADOW_MAPPING

#include "AppBase.h"
#include "Scene.h"
#include "ResourcesShadow.h"
#include "ResourcesShadow.h"
#include "ResourcesLight.h"
#include "PipelineSkybox.h"
#include "PipelineClear.h"
#include "PipelineFinish.h"
#include "PipelinePBRShadow.h"
#include "PipelineShadow.h"
#include "PipelineTonemap.h"
#include "PipelineResolveMS.h"
#include "PipelineLightRender.h"
#include "PipelineImGui.h"

// STL
#include <memory>

/*
Demo for shadow mapping with bindless textures
*/
class AppPBRShadow final : AppBase
{
public:
	AppPBRShadow();
	void MainLoop() override;
	void UpdateUBOs() override;
	void UpdateUI() override;

	void Init();
	void InitLights();

private:
	PipelineShadow* shadowPtr_;
	PipelineLightRender* lightPtr_;
	PipelinePBRShadow* pbrOpaquePtr_;
	PipelinePBRShadow* pbrTransparentPtr_;
	PipelineImGui* imguiPtr_;

	ResourcesLight* resLight_;
	ResourcesShadow* resShadow_;

	std::unique_ptr<Scene> scene_;
};

#endif