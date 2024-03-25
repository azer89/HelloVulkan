#ifndef APP_PBR_SLOT_BASED
#define APP_PBR_SLOT_BASED

#include "AppBase.h"
#include "Model.h"
#include "PipelineClear.h"
#include "PipelineImGui.h"
#include "ResourcesLight.h"
#include "PipelineSkybox.h"
#include "PipelineFinish.h"
#include "PipelineTonemap.h"
#include "PipelineResolveMS.h"
#include "PipelineLightRender.h"
#include "PipelineInfiniteGrid.h"
#include "PipelinePBRSlotBased.h"


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
	PipelineImGui* imguiPtr_ = nullptr;
	ResourcesLight* resLights_ = nullptr;
	std::unique_ptr<Model> model_ = nullptr;
};

#endif