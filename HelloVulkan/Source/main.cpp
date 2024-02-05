#include "AppPBR.h"
#include "AppPBRClusterForward.h"

// Entry point
int main()
{
	AppPBR app;
	//AppPBRClusterForward app;
	
	auto returnValue = app.MainLoop();
	return returnValue;
}