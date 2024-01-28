#version 460 core

/*
Fragment shader for skybox
*/

layout(location = 0) in vec3 direction;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 1) uniform samplerCube cubeMap;

void main()
{
	vec3 color = texture(cubeMap, direction).rgb;

	fragColor = vec4(color, 1.0);
};