#version 460 core

#include <InfiniteGrid/Params.glsl>
#include <InfiniteGrid/Header.glsl>

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec2 inCameraPos;
layout (location = 0) out vec4 fragColor;

void main()
{
	fragColor = GridColor(inUV, inCameraPos);
}