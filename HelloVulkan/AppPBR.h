#ifndef APP_PBR
#define APP_PBR

#include "AppBase.h"
#include "RendererCube.h"
#include "RendererClear.h"
#include "RendererFinish.h"
#include "RendererPBR.h"

#include <memory>

class AppPBR : AppBase
{
public:
	AppPBR();
	int MainLoop() override;
	void UpdateUBO(uint32_t imageIndex) override;

private:
	std::unique_ptr<RendererCube> cubePtr;
	std::unique_ptr<RendererClear> clearPtr;
	std::unique_ptr<RendererFinish> finishPtr;
	std::unique_ptr<RendererPBR> pbrPtr;

	float modelRotation;
};

#endif