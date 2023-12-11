#ifndef APP_TEST
#define APP_TEST

#include "AppBase.h"
#include "RendererCube.h"

#include <memory>

class AppTest : AppBase
{
public:
	AppTest();
	int MainLoop() override;
	void ComposeFrame(uint32_t imageIndex, const std::vector<RendererBase*>& renderers) override;

private:
	std::unique_ptr<RendererCube> cubePtr;
	
};

#endif