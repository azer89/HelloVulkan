# version 460 core

layout(location = 0) in vec3 direction;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform samplerCube texture1;

void main()
{
	vec3 envColor = texture(texture1, direction).rgb;

	// HDR tonemap and gamma correct
	envColor = envColor / (envColor + vec3(1.0));
	envColor = pow(envColor, vec3(1.0 / 2.2));

	fragColor = vec4(envColor, 1.0);
};
