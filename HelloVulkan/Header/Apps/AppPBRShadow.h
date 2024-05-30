#ifndef APP_PBR_SHADOW_MAPPING
#define APP_PBR_SHADOW_MAPPING

#include "AppBase.h"
#include "Scene.h"
#include "PipelineImGui.h"
#include "ResourcesLight.h"
#include "ResourcesShadow.h"
#include "ResourcesGBuffer.h"
#include "PipelineGBuffer.h"
#include "PipelineSSAO.h"
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