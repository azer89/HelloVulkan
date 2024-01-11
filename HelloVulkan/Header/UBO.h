#ifndef UNIFORM_BUFFER_OBJECTS
#define UNIFORM_BUFFER_OBJECTS

#include "glm/gtc/matrix_transform.hpp"

struct PerFrameUBO
{
	glm::mat4 cameraProjection;
	glm::mat4 cameraView;
	glm::vec4 cameraPosition;
};

struct ModelUBO
{
	glm::mat4 model;
};

#endif
