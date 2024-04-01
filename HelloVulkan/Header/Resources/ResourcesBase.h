#ifndef RESOURCES_BASE
#define RESOURCES_BASE

#include "UIData.h"

class ResourcesBase
{
public:
	virtual ~ResourcesBase() = default;
	virtual void Destroy() = 0;

	virtual void UpdateFromInputContext(VulkanContext& ctx, UIData& inputContext)
	{
	}
};

#endif