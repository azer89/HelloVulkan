#version 460 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragOffset;
layout(location = 1) in vec4 circleColor;

layout(location = 0) out vec4 fragColor;

void main()
{
	float dist = sqrt(dot(fragOffset, fragOffset));

	vec4 finalColor = circleColor;
	if (dist >= 0.8)
	{
		finalColor.a = 0;
	}

	float glow = max(1.0 - dist, 0.0) * 0.5;

	finalColor += glow;

	if (dist >= 1.0)
	{
		finalColor.a = 0;
	}

	fragColor = finalColor;
}