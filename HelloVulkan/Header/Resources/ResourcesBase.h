#ifndef RESOURCES_BASE
#define RESOURCES_BASE

#include "InputContext.h"

class ResourcesBase
{
public:
	virtual ~ResourcesBase() = default;
	virtual void Destroy() = 0;

	virtual void GetUpdateFromInputContext(VulkanContext& ctx, InputContext& inputContext)
	{
	}
};

#endif