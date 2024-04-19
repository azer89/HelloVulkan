#version 460 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragOffset;
layout(location = 1) in vec4 circleColor;

layout(location = 0) out vec4 fragColor;

const float ORB_SIZE = 0.7;
const float GLOW_FALLOFF = 2.0;
const float GLOW_STRENGTH = 0.5;

void main()
{
	float dist = sqrt(dot(fragOffset, fragOffset));

	vec4 finalColor = circleColor;

	if (dist >= ORB_SIZE)
	{
		finalColor.a = 0;
	}

	float glow = pow(max(1.0 - pow(dist, GLOW_FALLOFF), 0.0), 2.0) * GLOW_STRENGTH;

	finalColor += glow;

	if (dist >= 1.0)
	{
		finalColor.a = 0;
	}

	fragColor = finalColor;
}