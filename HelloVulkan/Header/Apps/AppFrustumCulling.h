#ifndef APP_FRUSTUM_CULLING
#define APP_FRUSTUM_CULLING

#include "AppBase.h"
#include "Scene.h"
#include "PipelineLine.h"
#include "PipelineImGui.h"
#include "ResourcesLight.h"
#include "PipelineAABBRender.h"
#include "PipelineLightRender.h"
#include "PipelineInfiniteGrid.h"
#include "PipelinePBRBindless.h"
#include "PipelineFrustumCulling.h"

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
	PipelineFrustumCulling* cullingPtr_{};
	PipelinePBRBindless* pbrPtr_{};
	PipelineImGui* imguiPtr_{};
	PipelineLine* linePtr_{};
	PipelineLightRender* lightPtr_{};
	PipelineInfiniteGrid* infGridPtr_{};
	PipelineAABBRender* boxRenderPtr_{};

	std::unique_ptr<Scene> scene_{};
	ResourcesLight* resourcesLight_{};

	bool updateFrustum_ = true;
};

#endif