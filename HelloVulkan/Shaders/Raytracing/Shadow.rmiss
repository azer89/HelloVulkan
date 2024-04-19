#version 460
#extension GL_EXT_ray_tracing : require

#include <Raytracing/Header/RayPayload.glsl>

layout(location = 1) rayPayloadInEXT bool shadowed;

void main()
{
	shadowed = false;
}