#ifndef APP_PBR_SHADOW_MAPPING
#define APP_PBR_SHADOW_MAPPING

#include "AppBase.h"
#include "Scene.h"
#include "PipelineImGui.h"
#include "PipelineClear.h"
#include "ResourcesLight.h"
#include "PipelineSkybox.h"
#include "PipelineFinish.h"
#include "ResourcesShadow.h"
#include "ResourcesShadow.h"
#include "PipelineShadow.h"
#include "PipelineTonemap.h"
#include "PipelinePBRShadow.h"
#include "PipelineResolveMS.h"
#include "PipelineLightRender.h"

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
	PipelineImGui* imguiPtr_ = nullptr;
	PipelineShadow* shadowPtr_ = nullptr;
	PipelineLightRender* lightPtr_ = nullptr;
	PipelinePBRShadow* pbrOpaquePtr_ = nullptr;
	PipelinePBRShadow* pbrTransparentPtr_ = nullptr;
	
	ResourcesLight* resLight_ = nullptr;
	ResourcesShadow* resShadow_ = nullptr;
	std::unique_ptr<Scene> scene_ = nullptr;
};

#endif