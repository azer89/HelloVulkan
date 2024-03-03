#ifndef APP_PBR_SHADOW_MAPPING
#define APP_PBR_SHADOW_MAPPING

#include "AppBase.h"
#include "VulkanImage.h"
#include "Light.h"
#include "Scene.h"
#include "ResourcesIBL.h"
#include "ResourcesShadow.h"

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
#include <vector>
#include <memory>

/*
Naive forward PBR
*/
class AppPBRShadow final : AppBase
{
public:
	AppPBRShadow();
	int MainLoop() override;
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

	std::unique_ptr<Scene> scene_;
	std::unique_ptr<Lights> lights_;
	std::unique_ptr<ResourcesShadow> resShadow_;

	ShadowMapUBO shadowUBO_;

	float cubemapMipmapCount_;

	float shadowNearPlane_;
	float shadowFarPlane_;
	uint32_t shadowMapSize_;
};

#endif