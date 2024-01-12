#version 460 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragOffset;

layout(location = 0) out vec4 fragColor;

void main()
{
	fragColor = vec4(1.0);
}