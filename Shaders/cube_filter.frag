# version 460 core

/*

Fragment shader to filter a cubemap into a diffuse cubemap or specular cubemap

This shader is based on:
https://github.com/KhronosGroup/glTF-IBL-Sampler
*/

layout(set = 0, binding = 0) uniform samplerCube cubeMap;

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 cubeFace0;
layout(location = 1) out vec4 cubeFace1;
layout(location = 2) out vec4 cubeFace2;
layout(location = 3) out vec4 cubeFace3;
layout(location = 4) out vec4 cubeFace4;
layout(location = 5) out vec4 cubeFace5;

#define MATH_PI 3.1415926535897932384626433832795
#define MATH_INV_PI (1.0 / MATH_PI)

// Enum
const uint cLambertian = 0;
const uint cGGX = 1;

layout(push_constant) uniform FilterParameters
{
	float roughness;
	float lodBias;
	uint sampleCount;
	uint currentMipLevel;
	uint width;
	uint distribution; // Enum
}
pcParams;

struct MicrofacetDistributionSample
{
	float pdf;
	float cosTheta;
	float sinTheta;
	float phi;
};

float Saturate(float v)
{
	return clamp(v, 0.0f, 1.0f);
}

// Hammersley Points on the Hemisphere
// CC BY 3.0 (Holger Dammertz)
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// with adapted interface
float RadicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// Hammersley2d describes a sequence of points in the 2d unit square [0,1)^2
// that can be used for quasi Monte Carlo integration
vec2 Hammersley2d(int i, int N)
{
	return vec2(float(i)/float(N), RadicalInverse_VdC(uint(i)));
}

// TBN generates a tangent bitangent normal coordinate frame from the normal
// (the normal must be normalized)
mat3 GenerateTBN(vec3 normal)
{
	vec3 bitangent = vec3(0.0, 1.0, 0.0);

	float NdotUp = dot(normal, vec3(0.0, 1.0, 0.0));
	float epsilon = 0.0000001;
	if (1.0 - abs(NdotUp) <= epsilon)
	{
		// Sampling +Y or -Y, so we need a more robust bitangent.
		if (NdotUp > 0.0)
		{
			bitangent = vec3(0.0, 0.0, 1.0);
		}
		else
		{
			bitangent = vec3(0.0, 0.0, -1.0);
		}
	}

	vec3 tangent = normalize(cross(bitangent, normal));
	bitangent = cross(normal, tangent);

	return mat3(tangent, bitangent, normal);
}

MicrofacetDistributionSample Lambertian(vec2 xi, float roughness)
{
	MicrofacetDistributionSample lambertian;

	// Cosine weighted hemisphere sampling
	// http://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations.html#Cosine-WeightedHemisphereSampling
	lambertian.cosTheta = sqrt(1.0 - xi.y);
	lambertian.sinTheta = sqrt(xi.y); // equivalent to `sqrt(1.0 - cosTheta*cosTheta)`;
	lambertian.phi = 2.0 * MATH_PI * xi.x;
	lambertian.pdf = lambertian.cosTheta / MATH_PI; // evaluation for solid angle, therefore drop the sinTheta

	return lambertian;
}

float D_GGX(float NdotH, float roughness)
{
	float a = NdotH * roughness;
	float k = roughness / (1.0 - NdotH * NdotH + a * a);
	return k * k * (1.0 / MATH_PI);
}

MicrofacetDistributionSample GGX(vec2 xi, float roughness)
{
	MicrofacetDistributionSample ggx;

	// Evaluate sampling equations
	float alpha = roughness * roughness;
	ggx.cosTheta = Saturate(sqrt((1.0 - xi.y) / (1.0 + (alpha * alpha - 1.0) * xi.y)));
	ggx.sinTheta = sqrt(1.0 - ggx.cosTheta * ggx.cosTheta);
	ggx.phi = 2.0 * MATH_PI * xi.x;

	// Evaluate GGX pdf (for half vector)
	ggx.pdf = D_GGX(ggx.cosTheta, alpha);

	// Apply the Jacobian to obtain a pdf that is parameterized by l
	// see https://bruop.github.io/ibl/
	// Typically you'd have the following:
	// float pdf = D_GGX(NoH, roughness) * NoH / (4.0 * VoH);
	// but since V = N => VoH == NoH
	ggx.pdf /= 4.0;

	return ggx;
}

// Mipmap Filtered Samples
float ComputeLod(float pdf)
{
	// https://cgg.mff.cuni.cz/~jaroslav/papers/2007-sketch-fis/Final_sap_0073.pdf
	return 0.5 * log2(6.0 * float(pcParams.width) * 
		float(pcParams.width) / (float(pcParams.sampleCount) * pdf));
}

// GetImportanceSample returns an importance sample direction with pdf in the .w component
vec4 GetImportanceSample(int sampleIndex, vec3 N, float roughness)
{
	// Generate a quasi monte carlo point in the unit square [0.1)^2
	vec2 xi = Hammersley2d(sampleIndex, int(pcParams.sampleCount));

	MicrofacetDistributionSample importanceSample;

	// Generate the points on the hemisphere with a fitting mapping for
	// the distribution (e.g. lambertian uses a cosine importance)
	if (pcParams.distribution == cLambertian)
	{
		importanceSample = Lambertian(xi, roughness);
	}
	else if (pcParams.distribution == cGGX)
	{
		// Trowbridge-Reitz / GGX microfacet model (Walter et al)
		// https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.html
		importanceSample = GGX(xi, roughness);
	}

	// Transform the hemisphere sample to the normal coordinate frame
	// i.e. rotate the hemisphere to the normal direction
	vec3 localSpaceDirection = normalize(vec3(
		importanceSample.sinTheta * cos(importanceSample.phi),
		importanceSample.sinTheta * sin(importanceSample.phi),
		importanceSample.cosTheta
	));

	mat3 TBN = GenerateTBN(N);
	vec3 direction = TBN * localSpaceDirection;

	return vec4(direction, importanceSample.pdf);
}

vec3 FilterColor(vec3 N)
{
	vec3 color = vec3(0.f);
	float weight = 0.0;

	for (int i = 0; i < int(pcParams.sampleCount); ++i)
	{
		vec4 importanceSample = GetImportanceSample(i, N, pcParams.roughness);

		vec3 H = vec3(importanceSample.xyz);
		float pdf = importanceSample.w;

		// Mipmap filtered samples (GPU Gems 3, 20.4)
		float lod = ComputeLod(pdf);

		// apply the bias to the lod
		lod += pcParams.lodBias;

		if (pcParams.distribution == cLambertian)
		{
			// Sample lambertian at a lower resolution to avoid fireflies
			vec3 lambertian = textureLod(cubeMap, H, lod).rgb;
			color += lambertian;
		}
		else if (pcParams.distribution == cGGX)
		{
			vec3 V = N;
			vec3 L = normalize(reflect(-V, H));
			float NdotL = dot(N, L);

			if (NdotL > 0.0)
			{
				if (pcParams.roughness == 0.0)
				{
					// without this the roughness=0 lod is too high
					lod = pcParams.lodBias;
				}
				vec3 sampleColor = textureLod(cubeMap, L, lod).rgb;
				color += sampleColor * NdotL;
				weight += NdotL;
			}
		}
	}

	if (weight > 0.0)
	{
		color /= weight;
	}
	else
	{
		color /= float(pcParams.sampleCount);
	}

	return color.rgb;
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
	vec2 texCoordNew = texCoord * float(1 << (pcParams.currentMipLevel));
	texCoordNew = texCoordNew * 2.0 - 1.0;

	for (int face = 0; face < 6; ++face)
	{
		vec3 scan = UVToXYZ(face, texCoordNew);
		vec3 direction = normalize(scan);
		WriteFace(face, FilterColor(direction));
	}
}