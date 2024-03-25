#ifndef APP_PBR_CLUSTER_FORWARD
#define APP_PBR_CLUSTER_FORWARD

#include "AppBase.h"
#include "Scene.h"
#include "ResourcesLight.h"
#include "ResourcesClusterForward.h"
#include "PipelineSkybox.h"
#include "PipelineClear.h"
#include "PipelineFinish.h"
#include "PipelineTonemap.h"
#include "PipelineResolveMS.h"
#include "PipelineLightRender.h"
#include "PipelineImGui.h"
#include "PipelinePBRClusterForward.h"
#include "PipelineAABBGenerator.h"
#include "PipelineLightCulling.h"

// STL
#include <memory>

/*
Clustered forward PBR
*/
class AppPBRClusterForward final : AppBase
{
public:
	AppPBRClusterForward();
	void MainLoop() override;
	void UpdateUBOs() override;
	void UpdateUI() override;

	void Init();
	void InitLights();

private:
	PipelineLightRender* lightPtr_;
	PipelineImGui* imguiPtr_;
	PipelinePBRClusterForward* pbrOpaquePtr_;
	PipelinePBRClusterForward* pbrTransparentPtr_;
	PipelineAABBGenerator* aabbPtr_;
	PipelineLightCulling* lightCullPtr_;

	ResourcesClusterForward* resCF_; 
	ResourcesLight* resLight_;

	std::unique_ptr<Scene> scene_;
};

#endif