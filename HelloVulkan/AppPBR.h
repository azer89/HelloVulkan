#ifndef APP_PBR
#define APP_PBR

#include "AppBase.h"
#include "RendererSkybox.h"
#include "RendererClear.h"
#include "RendererFinish.h"
#include "RendererPBR.h"
#include "RendererEquirect2Cubemap.h"

#include <memory>

class AppPBR : AppBase
{
public:
	AppPBR();
	int MainLoop() override;
	void UpdateUBO(uint32_t imageIndex) override;

private:
	std::unique_ptr<RendererSkybox> skyboxPtr;
	std::unique_ptr<RendererClear> clearPtr;
	std::unique_ptr<RendererFinish> finishPtr;
	std::unique_ptr<RendererPBR> pbrPtr;
	std::unique_ptr<RendererEquirect2Cubemap> e2cPtr;

	float modelRotation;
};

#endif