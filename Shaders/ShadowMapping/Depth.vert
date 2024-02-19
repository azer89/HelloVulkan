#version 460 core

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inNormal;
layout(location = 2) in vec4 inUV;

layout(set = 0, binding = 0)
#include <ShadowMapping//UBO.frag>

layout(set = 0, binding = 1) uniform ModelUBO
{
	mat4 model;
};

void main()
{
	gl_Position = shadowUBO.lightSpaceMatrix * model * vec4(inPosition.xyz, 1.0);
}