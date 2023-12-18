# version 460 core

layout(location = 0) in vec4 positionIn;
layout(location = 1) in vec4 normalIn;
layout(location = 2) in vec4 uvIn;

layout(location = 0) out vec3 worldPos;
layout(location = 1) out vec2 texCoord;
layout(location = 2) out vec3 normal;

layout(binding = 0) uniform PerFrameUBO
{
	mat4 cameraProjection;
	mat4 cameraView;
	vec4 cameraPosition;
}
frameUBO;

layout(binding = 1) uniform ModelUBO
{
	mat4 model;
}
modelUBO;

void main()
{
	worldPos = (modelUBO.model * positionIn).xyz;

	texCoord = uvIn.xy;

	mat3 normalMatrix = transpose(inverse(mat3(modelUBO.model)));
	normal = normalMatrix * normalIn.xyz;

	mat4 mvp = frameUBO.cameraProjection * frameUBO.cameraView * modelUBO.model;
	gl_Position =  mvp * positionIn;
}
