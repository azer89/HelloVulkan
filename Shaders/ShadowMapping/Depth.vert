#version 460 core

//layout(location = 0) in vec3 aPos;
//uniform mat4 lightSpaceMatrix;
//uniform mat4 model;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inNormal;
layout(location = 2) in vec4 inUV;

layout(set = 0, binding = 0) uniform ShadowMapUBO
{
	mat4 lightSpaceMatrix;
};

layout(set = 0, binding = 1) uniform ModelUBO
{
	mat4 model;
};

void main()
{
	gl_Position = lightSpaceMatrix * model * vec4(inPosition.xyz, 1.0);
}