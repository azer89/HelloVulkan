#ifndef APP_TEST
#define __APPAPP_TEST_BOXES_H__

#include "AppBase.h"

class AppTest : AppBase
{
public:
	AppTest();
	int MainLoop() override;
};

#endif