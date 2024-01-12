#version 460 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec2 fragOffset;
layout(location = 1) out vec4 circleColor;

layout(binding = 0) uniform PerFrameUBO
{
	mat4 cameraProjection;
	mat4 cameraView;
	vec4 cameraPosition;
} frameUBO;

struct LightData
{
	vec4 position;
	vec4 color;
};

layout(binding = 1) readonly buffer Lights { LightData data []; } inLights;

const vec2 OFFSETS[6] = vec2[](
	vec2(-1.0, -1.0),
	vec2(-1.0,  1.0),
	vec2( 1.0, -1.0),
	vec2( 1.0, -1.0),
	vec2(-1.0,  1.0),
	vec2( 1.0,  1.0)
);
const float RADIUS = 0.2;

void main()
{
	LightData light = inLights.data[gl_InstanceIndex];

	vec3 camRight = vec3(frameUBO.cameraView[0][0], frameUBO.cameraView[1][0], frameUBO.cameraView[2][0]);
	vec3 camUp = vec3(frameUBO.cameraView[0][1], frameUBO.cameraView[1][1], frameUBO.cameraView[2][1]);

	fragOffset = OFFSETS[gl_VertexIndex];

	vec3 positionWorld = light.position.xyz +
		RADIUS * fragOffset.x * camRight +
		RADIUS * fragOffset.y * camUp;

	circleColor = light.color;
	gl_Position = frameUBO.cameraProjection * frameUBO.cameraView * vec4(positionWorld, 1.0);
}