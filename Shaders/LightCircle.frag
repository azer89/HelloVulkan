#version 460 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragOffset;
layout(location = 1) in vec4 circleColor;

layout(location = 0) out vec4 fragColor;

void main()
{
	float dist = sqrt(dot(fragOffset, fragOffset));
	if (dist >= 1.0)
	{
		discard;
	}

	// Blurry edge
	float alpha = 1.0 - pow(dist, 5.0);

	fragColor = vec4(circleColor.xyz, alpha);
}