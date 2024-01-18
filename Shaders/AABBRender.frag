#version 460 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 cubeColor;

layout(location = 0) out vec4 fragColor;

void main()
{
	fragColor = cubeColor;
}