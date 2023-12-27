#version 460 core

/*
Fragment shader for skybox
*/

layout(location = 0) in vec3 direction;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform samplerCube cubeMap;

void main()
{
	vec3 color = texture(cubeMap, direction).rgb;

	// HDR tonemap and gamma correct
	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0 / 2.2));

	fragColor = vec4(color, 1.0);
};
