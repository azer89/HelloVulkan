#ifndef UNIFORM_BUFFER_OBJECTS
#define UNIFORM_BUFFER_OBJECTS

#include "glm/glm.hpp"

// TODO Separate this into multiple UBOs
struct CameraUBO
{
	alignas(16)
	glm::mat4 projection;
	alignas(16)
	glm::mat4 view;
	alignas(16)
	glm::vec4 position;
	alignas(4)
	float near;
	alignas(4)
	float far;
};

struct ModelUBO
{
	alignas(16)
	glm::mat4 model;
};

#endif
