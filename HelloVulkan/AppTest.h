#ifndef APP_TEST
#define APP_TEST

#include "AppBase.h"

class AppTest : AppBase
{
public:
	AppTest();
	int MainLoop() override;
};

#endif