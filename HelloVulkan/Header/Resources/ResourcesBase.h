#ifndef RESOURCES_BASE
#define RESOURCES_BASE

#include "UIData.h"

class ResourcesBase
{
public:
	virtual ~ResourcesBase() = default;
	virtual void Destroy() = 0;

	virtual void UpdateFromUIData(VulkanContext& ctx, UIData& uiData)
	{
	}
};

#endif