#include "AppPBRSlotBased.h"
#include "AppPBRBindless.h"
#include "AppPBRShadowMapping.h"
#include "AppPBRClusterForward.h"
#include "AppSimpleRaytracing.h"

// Entry point
int main()
{
	// The good ol resource binding per draw call
	//AppPBRSlotBased app;
	
	// Bind resource once and render using draw indirect and dsecriptor indexing
	//AppPBRBindless app;
	
	//AppPBRShadowMapping app;
	
	AppPBRClusterForward app;
	
	// Currently can only draw a triangle
	//AppSimpleRaytracing app;
	
	auto returnValue = app.MainLoop();
	return returnValue;
}