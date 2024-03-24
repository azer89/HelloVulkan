#ifndef APP_PBR_SLOT_BASED
#define APP_PBR_SLOT_BASED

#include "AppBase.h"
#include "Model.h"
#include "ResourcesLight.h"
#include "PipelineSkybox.h"
#include "PipelineClear.h"
#include "PipelineFinish.h"
#include "PipelinePBRSlotBased.h"
#include "PipelineTonemap.h"
#include "PipelineResolveMS.h"
#include "PipelineLightRender.h"
#include "PipelineInfiniteGrid.h"
#include "PipelineImGui.h"

// STL
#include <memory>

/*
Naive forward PBR
*/
class AppPBRSlotBased final : AppBase
{
public:
	AppPBRSlotBased();
	void MainLoop() override;
	void UpdateUBOs() override;
	void UpdateUI() override;

	void Init();
	void InitLights();

private:
	PipelineImGui* imguiPtr_;
	ResourcesLight* resLights_;
	std::unique_ptr<Model> model_;
};

#endif