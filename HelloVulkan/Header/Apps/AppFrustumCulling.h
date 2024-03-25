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

private:
	PipelineLightRender* lightPtr_;
	PipelineFrustumCulling* cullingPtr_;
	PipelinePBRBindless* pbrPtr_;
	PipelineImGui* imguiPtr_;
	PipelineLine* linePtr_;
	PipelineInfiniteGrid* infGridPtr_;
	PipelineAABBRender* boxRenderPtr_;

	std::unique_ptr<Scene> scene_;
	ResourcesLight* resourcesLight_;

	bool updateFrustum_;
};

#endif