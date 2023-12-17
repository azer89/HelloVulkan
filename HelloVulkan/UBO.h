#ifndef UNIFORM_BUFFER_OBJECTS
#define UNIFORM_BUFFER_OBJECTS

#include "glm/gtc/matrix_transform.hpp"

struct PerFrameUBO
{
	glm::mat4 cameraProjection;
	glm::mat4 cameraView;
	glm::mat4 model;
	glm::vec4 cameraPosition;
};

#endif
