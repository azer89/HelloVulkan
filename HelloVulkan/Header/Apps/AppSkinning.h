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
	PipelineImGui* imguiPtr_{};
	PipelineShadow* shadowPtr_{};
	PipelineGBuffer* gPtr_{};
	PipelineSSAO* ssaoPtr_{};
	PipelinePBRShadow* pbrOpaquePtr_{};
	PipelinePBRShadow* pbrTransparentPtr_{};
	PipelineLightRender* lightPtr_{};

	std::unique_ptr<Scene> scene_{};
	ResourcesLight* resourcesLight_{};
	ResourcesShadow* resourcesShadow_{};
	ResourcesGBuffer* resourcesGBuffer_{};
};

#endif