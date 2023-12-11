#ifndef APP_TEST
#define APP_TEST

#include "AppBase.h"
#include "RendererCube.h"
#include "RendererClear.h"
#include "RendererFinish.h"
#include "RendererPBR.h"

#include <memory>

class AppTest : AppBase
{
public:
	AppTest();
	int MainLoop() override;
	void ComposeFrame(uint32_t imageIndex, const std::vector<RendererBase*>& renderers) override;

private:
	std::unique_ptr<RendererCube> cubePtr;
	std::unique_ptr<RendererClear> clearPtr;
	std::unique_ptr<RendererFinish> finishPtr;
	std::unique_ptr<RendererPBR> pbrPtr;
};

#endif