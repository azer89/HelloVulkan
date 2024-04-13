#ifndef APP_SKINNING
#define APP_SKINNING

#include "AppBase.h"
#include "Scene.h"
#include "ResourcesLight.h"
#include "ResourcesShadow.h"
#include "ResourcesGBuffer.h"
#include "PipelineGBuffer.h"
#include "PipelineSSAO.h"
#include "PipelineImGui.h"
#include "PipelineShadow.h"
#include "PipelinePBRShadow.h"
#include "PipelineLightRender.h"
#include "PipelineInfiniteGrid.h"

#include <memory>

class AppSkinning final : AppBase
{
public:
	AppSkinning();
	void MainLoop() override;
	void UpdateUBOs() override;
	void UpdateUI() override;

	void Init();
	void InitScene();
	void InitLights();

private:
	PipelineImGui* imguiPtr_ = nullptr;
	PipelineShadow* shadowPtr_ = nullptr;
	PipelineGBuffer* gPtr_ = nullptr;
	PipelineSSAO* ssaoPtr_ = nullptr;
	PipelinePBRShadow* pbrOpaquePtr_ = nullptr;
	PipelinePBRShadow* pbrTransparentPtr_ = nullptr;
	PipelineLightRender* lightPtr_ = nullptr;

	std::unique_ptr<Scene> scene_ = nullptr;
	ResourcesLight* resourcesLight_ = nullptr;
	ResourcesShadow* resourcesShadow_ = nullptr;
	ResourcesGBuffer* resourcesGBuffer_ = nullptr;
};

#endif