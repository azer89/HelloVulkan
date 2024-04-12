#ifndef APP_PBR_SHADOW_MAPPING
#define APP_PBR_SHADOW_MAPPING

#include "AppBase.h"
#include "Scene.h"
#include "PipelineImGui.h"
#include "ResourcesLight.h"
#include "ResourcesShadow.h"
#include "ResourcesGBuffer.h"
#include "PipelineGBuffer.h"
#include "PipelineShadow.h"
#include "PipelinePBRShadow.h"
#include "PipelineLightRender.h"

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
	PipelineGBuffer* gPtr_ = nullptr;
	PipelinePBRShadow* pbrOpaquePtr_ = nullptr;
	PipelinePBRShadow* pbrTransparentPtr_ = nullptr;
	PipelineLightRender* lightPtr_ = nullptr;
	
	std::unique_ptr<Scene> scene_ = nullptr;
	ResourcesLight* resourcesLight_ = nullptr;
	ResourcesShadow* resourcesShadow_ = nullptr;
	ResourcesGBuffer* resourcesGBuffer_ = nullptr;
};

#endif