#ifndef RESOURCES_BASE
#define RESOURCES_BASE

#include "InputContext.h"

class ResourcesBase
{
public:
	virtual void GetUpdateFromInputContext(InputContext* inputContext)
	{
	}
};

#endif