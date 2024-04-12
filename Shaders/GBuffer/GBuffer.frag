#version 460 core
#extension GL_EXT_buffer_reference : require

layout(location = 0) in vec3 viewPos;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec3 gNormal;

void main()
{
	gPosition = vec4(viewPos, 1.0);
	gNormal = normalize(normal);
}