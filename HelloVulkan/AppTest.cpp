#include "AppTest.h"

AppTest::AppTest()
{
}

int AppTest::MainLoop()
{
	while (!GLFWWindowShouldClose())
	{
		PollEvents();
		ProcessTiming();
		ProcessInput();
	}

	Terminate();

	return 0;
}