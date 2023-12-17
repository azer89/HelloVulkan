# version 460 core

layout(location = 0) in vec4 positionIn;
layout(location = 1) in vec4 normalIn;
layout(location = 2) in vec4 uvIn;

layout(location = 0) out vec3 worldPos;
layout(location = 1) out vec2 texCoord;
layout(location = 2) out vec3 normal;

/*layout(binding = 0) uniform UniformBuffer
{
	mat4 mvp;
	mat4 mv;
	mat4 m;
	vec4 cameraPos;
}
ubo;*/
layout(binding = 0) uniform PerFrameUBO
{
	mat4 cameraProjection;
	mat4 cameraView;
	mat4 model;
	vec4 cameraPosition;
}
ubo;

void main()
{
	worldPos = (ubo.model * positionIn).xyz;

	texCoord = uvIn.xy;

	mat3 normalMatrix = transpose(inverse(mat3(ubo.model)));
	normal = normalMatrix * normalIn.xyz;

	//mat4 mvp = ubo.cameraProjection * ubo.cameraView * ubo.model;
	gl_Position =  ubo.cameraProjection * ubo.cameraView * ubo.model * positionIn;
}
