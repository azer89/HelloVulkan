#include "AppPBRSlotBased.h"
#include "AppPBRBindless.h"
#include "AppPBRShadow.h"
#include "AppPBRClusterForward.h"
#include "AppSimpleRaytracing.h"

// Entry point
int main()
{
	// Bind resource once and render using draw indirect and descriptor indexing
	AppPBRBindless app;

	// The good ol resource binding per draw call
	//AppPBRSlotBased app;
	
	//AppPBRShadow app;
	
	//AppPBRClusterForward app;
	
	// Currently can only draw a triangle
	//AppSimpleRaytracing app;
	
	auto returnValue = app.MainLoop();
	return returnValue;
}