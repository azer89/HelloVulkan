#ifndef APP_PBR_SHADOW_MAPPING
#define APP_PBR_SHADOW_MAPPING

#include "AppBase.h"
#include "Scene.h"
#include "ResourcesShadow.h"
#include "ResourcesLight.h"

// Pipelines
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
Naive forward PBR
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
	void DestroyResources();

private:
	std::unique_ptr<PipelineClear> clearPtr_;
	std::unique_ptr<PipelineSkybox> skyboxPtr_;
	std::unique_ptr<PipelineShadow> shadowPtr_;
	std::unique_ptr<PipelinePBRShadow> pbrPtr_;
	std::unique_ptr<PipelineTonemap> tonemapPtr_;
	std::unique_ptr<PipelineFinish> finishPtr_;
	std::unique_ptr<PipelineResolveMS> resolveMSPtr_;
	std::unique_ptr<PipelineLightRender> lightPtr_;
	std::unique_ptr<PipelineImGui> imguiPtr_;

	std::unique_ptr<ResourcesLight> resLights_;
	std::unique_ptr<ResourcesShadow> resShadow_;

	std::unique_ptr<Scene> scene_;

	ShadowMapUBO shadowUBO_;
	float cubemapMipmapCount_;
};

#endif