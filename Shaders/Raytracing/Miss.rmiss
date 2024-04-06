#version 460
#extension GL_EXT_ray_tracing : enable

#include <Raytracing/RayPayload.glsl>

layout(location = 0) rayPayloadInEXT vec3 hitValue;
layout(location = 1) rayPayloadEXT RayPayload rayPayload;

const float SKY_INTENSITY = 7.5;

void main()
{
	// New code
	const float t = 0.5 * (normalize(gl_WorldRayDirectionEXT).y + 1.0);
	const vec3 gradientStart = vec3(0.5, 0.6, 1.0);
	const vec3 gradientEnd = vec3(1.0);
	const vec3 skyColor = mix(gradientEnd, gradientStart, t);

	rayPayload.color = skyColor * SKY_INTENSITY;
	rayPayload.distance = -1.0;

	// Old code
	//hitValue = vec3(1.0, 1.0, 1.0);
}