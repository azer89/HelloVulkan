#include "AppPBRSlotBased.h"
#include "AppPBRBindless.h"
#include "AppPBRShadow.h"
#include "AppFrustumCulling.h"
#include "AppPBRClusterForward.h"
#include "AppSimpleRaytracing.h"

// Entry point
int main()
{
	// Bindless, using draw indirect, buffer device address, and descriptor indexing
	//AppPBRBindless app;

	// Shadow demo, using draw indirect, buffer device address, and descriptor indexing
	AppPBRShadow app;

	// The good ol resource binding per draw call
	//AppPBRSlotBased app;
	
	//AppFrustumCulling app;

	//AppPBRClusterForward app;
	
	//AppSimpleRaytracing app;
	
	app.MainLoop();

	return 0;
}