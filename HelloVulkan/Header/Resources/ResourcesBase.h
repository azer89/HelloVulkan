#ifndef RESOURCES_BASE
#define RESOURCES_BASE

#include "UIData.h"

class ResourcesBase
{
public:
	virtual ~ResourcesBase() = default;
	virtual void Destroy() = 0;

	// If the window is resized
	virtual void OnWindowResized(VulkanContext& ctx)
	{
	}

	virtual void UpdateFromUIData(VulkanContext& ctx, UIData& uiData)
	{
	}
};

#endif