#version 460 core

layout(location = 0) in vec4 lineColor;
layout(location = 0) out vec4 outColor;

void main()
{
	outColor = lineColor;
}