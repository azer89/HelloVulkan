#include "AppPBRSlotBased.h" // The good ol resource binding per draw call
#include "AppPBRBindless.h" // Bindless, using draw indirect, buffer device address, and descriptor indexing
#include "AppPBRShadow.h" // Shadow demo, using draw indirect, buffer device address, and descriptor indexing
#include "AppFrustumCulling.h"
#include "AppPBRClusterForward.h"
#include "AppSimpleRaytracing.h"
#include "AppSkinning.h"

// Entry point
int main()
{
	AppSimpleRaytracing app;
	app.MainLoop();
	return 0;
}