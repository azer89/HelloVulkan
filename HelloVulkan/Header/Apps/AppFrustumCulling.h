#ifndef APP_FRUSTUM_CULLING
#define APP_FRUSTUM_CULLING

#include "AppBase.h"
#include "Scene.h"

// Pipelines
#include "PipelineSkybox.h"
#include "PipelineClear.h"
#include "PipelineFinish.h"
#include "PipelineTonemap.h"
#include "PipelineResolveMS.h"
#include "PipelineLightRender.h"
#include "PipelineInfiniteGrid.h"
#include "PipelineImGui.h"

#include <memory>

class AppFrustumCulling final : AppBase
{
public:
	AppFrustumCulling();
	void MainLoop() override;
	void UpdateUBOs() override;
	void UpdateUI() override;

	void Init();
	void InitLights();
	void DestroyResources();

private:
	std::unique_ptr<PipelineClear> clearPtr_;
	std::unique_ptr<PipelineSkybox> skyboxPtr_;
	std::unique_ptr<PipelineTonemap> tonemapPtr_;
	std::unique_ptr<PipelineFinish> finishPtr_;
	std::unique_ptr<PipelineResolveMS> resolveMSPtr_;
	std::unique_ptr<PipelineLightRender> lightPtr_;
	std::unique_ptr<PipelineImGui> imguiPtr_;
	std::unique_ptr<PipelineInfiniteGrid> infGridPtr_;

	std::unique_ptr<ResourcesLight> resLights_;
	std::unique_ptr<Model> model_;

	float cubemapMipmapCount_;
	float modelRotation_;
};

#endif