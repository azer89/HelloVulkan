#version 460 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragOffset;
layout(location = 1) in vec4 circleColor;

layout(location = 0) out vec4 fragColor;

void main()
{
	float dist = sqrt(dot(fragOffset, fragOffset));

	/*float alpha = 1.0 - pow(dist, 5.0);
	//float alpha = 1.0;
	if (dist >= 1.0)
	{
		alpha = 0;
	}

	float glow = clamp(0.2 / dist, 0.0, 0.45);

	vec4 finalColor = vec4(circleColor.xyz + glow, alpha);

	fragColor = finalColor;*/

	vec4 finalColor = circleColor;
	if (dist >= 0.8)
	{
		finalColor.a = 0;
	}

	//float glow = clamp(0.2 / dist, 0.0, 0.45);
	float glow = max(1.0 - dist, 0.0) * 0.75;

	finalColor += glow;

	if (dist >= 1.0)
	{
		finalColor.a = 0;
	}

	fragColor = finalColor;
}