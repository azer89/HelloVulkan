#ifndef APP_FRUSTUM_CULLING
#define APP_FRUSTUM_CULLING

#include "AppBase.h"
#include "Scene.h"
#include "ResourcesLight.h"
#include "PipelineSkybox.h"
#include "PipelineClear.h"
#include "PipelineFinish.h"
#include "PipelineTonemap.h"
#include "PipelineFrustumCulling.h"
#include "PipelinePBRBindless.h"
#include "PipelineResolveMS.h"
#include "PipelineLightRender.h"
#include "PipelineInfiniteGrid.h"
#include "PipelineImGui.h"
#include "PipelineLine.h"
#include "PipelineInfiniteGrid.h"
#include "PipelineAABBRender.h"

#include <memory>

class AppFrustumCulling final : AppBase
{
public:
	AppFrustumCulling();
	void MainLoop() override;
	void UpdateUBOs() override;
	void UpdateUI() override;

	void Init();
	void InitScene();
	void InitLights();
	void DestroyResources();

private:
	std::unique_ptr<PipelineClear> clearPtr_;
	std::unique_ptr<PipelineSkybox> skyboxPtr_;
	std::unique_ptr<PipelineTonemap> tonemapPtr_;
	std::unique_ptr<PipelineFinish> finishPtr_;
	std::unique_ptr<PipelineResolveMS> resolveMSPtr_;
	std::unique_ptr<PipelineInfiniteGrid> infGridPtr_;
	std::unique_ptr<PipelineLightRender> lightPtr_;
	std::unique_ptr<PipelineFrustumCulling> cullingPtr_;
	std::unique_ptr<PipelinePBRBindless> pbrPtr_;
	std::unique_ptr<PipelineImGui> imguiPtr_;
	std::unique_ptr<PipelineLine> linePtr_;
	std::unique_ptr<PipelineAABBRender> boxRenderPtr_;
	
	std::unique_ptr<ResourcesLight> resLight_;

	std::unique_ptr<Scene> scene_;
	std::unique_ptr<ResourcesLight> resourcesLight_; // TODO Set as unique_ptr

	bool updateFrustum_;

	// TODO Move to ResourcesIBL
	float cubemapMipmapCount_;
};

#endif