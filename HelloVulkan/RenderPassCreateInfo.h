#ifndef RENDER_PASS_CREATE_INFO
#define RENDER_PASS_CREATE_INFO

#include <cstdint>

struct RenderPassCreateInfo final
{
	bool clearColor_ = false;
	bool clearDepth_ = false;
	uint8_t flags_ = 0;
};

#endif