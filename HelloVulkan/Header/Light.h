#ifndef LIGHT
#define LIGHT

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

struct Light
{
	glm::vec4 position_;
	glm::vec4 color_;
};

#endif