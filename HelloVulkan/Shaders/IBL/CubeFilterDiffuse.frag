#version 460 core

/*
Fragment shader to filter a cubemap into a diffuse (irradiance) cubemap

This shader is based on 
[1] https://learnopengl.com/
[2] https://github.com/SaschaWillems/Vulkan-glTF-PBR
*/

layout(location = 0) in vec2 texCoord;

// Multiple render targets
layout(location = 0) out vec4 cubeFace0;
layout(location = 1) out vec4 cubeFace1;
layout(location = 2) out vec4 cubeFace2;
layout(location = 3) out vec4 cubeFace3;
layout(location = 4) out vec4 cubeFace4;
layout(location = 5) out vec4 cubeFace5;

layout(set = 0, binding = 0) uniform samplerCube cubeMap;

#define PI 3.1415926535897932384626433832795

// These push constants are only used for specular map
layout(push_constant) uniform PushConstantCubeFilter
{
	float roughness;
	uint sampleCount;
}
pcParams;

vec3 Diffuse(vec3 N)
{
	const float TWO_PI = PI * 2.0;
	const float HALF_PI = PI * 0.5;

	vec3 diffuseColor = vec3(0.0);

	// Tangent space calculation from origin point
	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = normalize(cross(up, N));
	up = normalize(cross(N, right));

	float sampleDelta = 0.025;
	uint sampleCount = 0u;
	for (float phi = 0.0; phi < TWO_PI; phi += sampleDelta)
	{
		for (float theta = 0.0; theta < HALF_PI; theta += sampleDelta)
		{
			// Spherical to cartesian (in tangent space)
			vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			// Tangent space to world
			vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

			diffuseColor += texture(cubeMap, sampleVec).rgb * cos(theta) * sin(theta);
			sampleCount++;
		}
	}
	diffuseColor = PI * diffuseColor * (1.0 / float(sampleCount));

	return diffuseColor;
}

void WriteFace(int face, vec3 colorIn)
{
	vec4 color = vec4(colorIn.rgb, 1.0f);

	if (face == 0)
	{
		cubeFace0 = color;
	}
	else if (face == 1)
	{
		cubeFace1 = color;
	}
	else if (face == 2)
	{
		cubeFace2 = color;
	}
	else if (face == 3)
	{
		cubeFace3 = color;
	}
	else if (face == 4)
	{
		cubeFace4 = color;
	}
	else
	{
		cubeFace5 = color;
	}
}

vec3 UVToXYZ(int face, vec2 uv)
{
	if (face == 0)
	{
		return vec3(1.f, uv.y, -uv.x);
	}
	else if (face == 1)
	{
		return vec3(-1.f, uv.y, uv.x);
	}
	else if (face == 2)
	{
		return vec3(+uv.x, -1.f, +uv.y);
	}
	else if (face == 3)
	{
		return vec3(+uv.x, 1.f, -uv.y);
	}
	else if (face == 4)
	{
		return vec3(+uv.x, uv.y, 1.f);
	}
	else
	{
		return vec3(-uv.x, +uv.y, -1.f);
	}
}

// Entry point
void main()
{
	vec2 texCoordNew = texCoord * 2.0 - 1.0;

	for (int face = 0; face < 6; ++face)
	{
		vec3 scan = UVToXYZ(face, texCoordNew);
		vec3 direction = normalize(scan);
		direction.y = -direction.y;
		WriteFace(face, Diffuse(direction));
	}
}