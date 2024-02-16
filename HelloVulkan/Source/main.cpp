#include "AppPBR.h"
#include "AppPBRClusterForward.h"
#include "AppSimpleRaytracing.h"

// Entry point
int main()
{
	//AppPBR app;
	AppPBRClusterForward app;
	//AppSimpleRaytracing app;
	
	auto returnValue = app.MainLoop();
	return returnValue;
}