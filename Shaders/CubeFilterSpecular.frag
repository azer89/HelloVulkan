#version 460 core

/*
Fragment shader to filter a cubemap into a specular (prefilter) cubemap

This shader is based on 
[1] https://learnopengl.com/
[2] http://www.codinglabs.net/article_physically_based_rendering.aspx
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

#include <Hammersley.glsl>
#include <PBRHeader.glsl>

layout(push_constant) uniform PushConstantCubeFilter
{
	float roughness;
	uint sampleCount;
}
pcParams;

vec3 Specular(vec3 N)
{
	// Make the simplifying assumption that V equals R equals the normal 
	vec3 R = N;
	vec3 V = R;

	vec2 texelSize = 1.0 / textureSize(cubeMap, 0);
	float saTexel = 4.0 * PI / (6.0 * texelSize.x * texelSize.x);

	vec3 specularColor = vec3(0.0);
	float totalWeight = 0.0;

	for (uint i = 0u; i < pcParams.sampleCount; ++i)
	{
		// Generates a sample vector that's biased towards
		// the preferred alignment direction (importance sampling).
		vec2 Xi = Hammersley(i, pcParams.sampleCount);
		vec3 H = ImportanceSampleGGX(Xi, N, pcParams.roughness);
		vec3 L = normalize(2.0 * dot(V, H) * H - V);

		float NoL = max(dot(N, L), 0.0);

		if (NoL > 0.0)
		{
			float NoH = max(dot(N, H), 0.0);
			float HoV = max(dot(H, V), 0.0);

			// Sample from the environment's mip level based on roughness/pdf
			float D = DistributionGGX(NoH, pcParams.roughness); // Trowbridge-Reitz GGX
			float pdf = D * NoH / (4.0 * HoV) + 0.0001;
			float saSample = 1.0 / (float(pcParams.sampleCount) * pdf + 0.0001);
			float mipLevel = pcParams.roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

			specularColor += textureLod(cubeMap, L, mipLevel).rgb * NoL;
			totalWeight += NoL;
		}
	}

	specularColor = specularColor / totalWeight;

	return specularColor;
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
		WriteFace(face, Specular(direction));
	}
}