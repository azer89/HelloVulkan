#ifndef APP_PBR_CLUSTER_FORWARD
#define APP_PBR_CLUSTER_FORWARD

#include "AppBase.h"
#include "Scene.h"
#include "ResourcesLight.h"
#include "PipelineLightRender.h"
#include "PipelineImGui.h"
#include "PipelineAABBGenerator.h"
#include "PipelineLightCulling.h"
#include "ResourcesClusterForward.h"
#include "PipelinePBRClusterForward.h"

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
	PipelineLightRender* lightPtr_ = nullptr;
	PipelineImGui* imguiPtr_ = nullptr;
	PipelinePBRClusterForward* pbrOpaquePtr_ = nullptr;
	PipelinePBRClusterForward* pbrTransparentPtr_ = nullptr;
	PipelineAABBGenerator* aabbPtr_ = nullptr;
	PipelineLightCulling* lightCullPtr_ = nullptr;

	ResourcesClusterForward* resCF_ = nullptr;
	ResourcesLight* resourcesLight_ = nullptr;
	std::unique_ptr<Scene> scene_ = nullptr;
};

#endif