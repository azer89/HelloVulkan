#version 460 core

layout(location = 0) in vec3 worldPos;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 viewPos;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PushConstantPBR
{
	float lightIntensity;
	float baseReflectivity;
	float maxReflectionLod;
	float attenuationF;
}
pc;

layout(set = 0, binding = 0) uniform PerFrameUBO
{
	mat4 cameraProjection;
	mat4 cameraView;
	vec4 cameraPosition;
}
frameUBO;

layout(set = 0, binding = 2) uniform ClusterForwardUBO
{
	mat4 cameraInverseProjection;
	mat4 cameraView;
	vec2 screenSize;
	float sliceScaling;
	float sliceBias;
	float cameraNear;
	float cameraFar;
	uint sliceCountX;
	uint sliceCountY;
	uint sliceCountZ;
}
cfUBO;

// SSBO
struct LightData
{
	vec4 position;
	vec4 color;
	float radius;
};
layout(set = 0, binding = 3) readonly buffer Lights { LightData data []; } inLights;

// SSBO
struct LightCell
{
	uint offset;
	uint count;
};
layout(set = 0, binding = 4) buffer LightCells
{
	LightCell data [];
}
lightCells;

// SSBO
layout(set = 0, binding = 5) buffer LightIndices
{
	uint data [];
}
lightIndices;

struct AABB
{
	vec4 minPoint;
	vec4 maxPoint;
};
layout(set = 0, binding = 6) buffer Clusters
{
	AABB data [];
}
clusters;

layout(set = 0, binding = 7) uniform sampler2D textureAlbedo;
layout(set = 0, binding = 8) uniform sampler2D textureNormal;
layout(set = 0, binding = 9) uniform sampler2D textureMetalness;
layout(set = 0, binding = 10) uniform sampler2D textureRoughness;
layout(set = 0, binding = 11) uniform sampler2D textureAO;
layout(set = 0, binding = 12) uniform sampler2D textureEmissive;

layout(set = 0, binding = 13) uniform samplerCube specularMap;
layout(set = 0, binding = 14) uniform samplerCube diffuseMap;
layout(set = 0, binding = 15) uniform sampler2D brdfLUT;

// Include files
#include <PBRHeader.frag>
#include <Hammersley.frag>

vec3 DEBUG_COLORS[8] = vec3[](
	vec3(0.04, 0.04, 0),
	vec3(0, 0, 0.08),
	vec3(0, 0.08, 0),
	vec3(0, 0.08, 0.08),
	vec3(0.08, 0, 0),
	vec3(0.08, 0, 0.08),
	vec3(0.08, 0.08, 0),
	vec3(0, 0.04, 0.04)
);

vec3 DEBUG_COLORS_2[8] = vec3[](
   vec3(1, 0, 0), vec3(0, 0, 1), vec3(0, 1, 0), vec3(0, 1, 1),
   vec3(1, 0, 0), vec3(1, 0, 1), vec3(1, 1, 0), vec3(1, 1, 1)
);


// Tangent-normals to world-space
vec3 GetNormalFromMap(vec3 tangentNormal, vec3 worldPos, vec3 normal, vec2 texCoord)
{
	vec3 Q1 = dFdx(worldPos);
	vec3 Q2 = dFdy(worldPos);
	vec2 st1 = dFdx(texCoord);
	vec2 st2 = dFdy(texCoord);

	vec3 N = normalize(normal);
	vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

float LinearDepth(float z)
{
	float zNear = cfUBO.cameraNear;
	float zFar = cfUBO.cameraFar;
	return zNear * zFar / (zFar + z * (zNear - zFar));
	//return ((-viewPos.z) - cfUBO.cameraNear) / (cfUBO.cameraFar - cfUBO.cameraNear);
}

float Attenuation(float d, float r)
{
	float f = pc.attenuationF;
	float s = d / r;
	float s2 = s * s;
	float nom = pow(1.0 - s2, 2.0);
	float denom = 1.0 + f * s2;
	return nom / denom;
}

vec3 Radiance(
	vec3 albedo,
	vec3 N,
	vec3 V,
	vec3 F0,
	float metallic,
	float roughness,
	float alphaRoughness,
	float NoV,
	vec3 lightPosition,
	vec3 lightColor)
{
	vec3 Lo = vec3(0.0);

	vec3 L = normalize(lightPosition - worldPos); // Incident light vector
	vec3 H = normalize(V + L); // Halfway vector
	float NoH = max(dot(N, H), 0.0);
	float NoL = max(dot(N, L), 0.0);
	float HoV = max(dot(H, V), 0.0);
	float distance = length(lightPosition - worldPos);
	float attenuation = 1.0 / pow(distance, pc.attenuationF);

	vec3 radiance = lightColor * attenuation * pc.lightIntensity;

	// Cook-Torrance BRDF
	float D = DistributionGGX(NoH, roughness);
	float G = GeometrySchlickGGX(NoL, NoV, alphaRoughness);
	vec3 F = FresnelSchlick(HoV, F0);

	vec3 numerator = D * G * F;
	float denominator = 4.0 * NoV * NoL + 0.0001; // + 0.0001 to prevent divide by zero
	vec3 specular = numerator / denominator;

	// kS is equal to Fresnel
	vec3 kS = F;
	// For energy conservation, the diffuse and specular light can't
	// be above 1.0 (unless the surface emits light); to preserve this
	// relationship the diffuse component (kD) should equal 1.0 - kS.
	vec3 kD = vec3(1.0) - kS;
	// Multiply kD by the inverse metalness such that only non-metals 
	// have diffuse lighting, or a linear blend if partly metal (pure metals
	// have no diffuse light).
	kD *= 1.0 - metallic;

	vec3 diffuse = Diffuse(kD * albedo);

	// Add to outgoing radiance Lo
	// Note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
	Lo += (diffuse + specular) * radiance * NoL;

	return Lo;
}

vec3 Ambient(
	vec3 albedo,
	vec3 F0,
	vec3 N,
	vec3 V,
	float metallic,
	float roughness,
	float ao,
	float NoV)
{
	// Ambient lighting (we now use IBL as the ambient term)
	vec3 F = FresnelSchlickRoughness(NoV, F0, roughness);

	vec3 kS = F;
	vec3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;

	vec3 irradiance = texture(diffuseMap, N).rgb;
	vec3 diffuse = irradiance * albedo;

	// Sample both the pre-filter map and the BRDF lut and combine them together as
	// per the Split-Sum approximation to get the IBL specular part.
	vec3 R = reflect(-V, N);
	vec3 prefilteredColor = textureLod(specularMap, R, roughness * pc.maxReflectionLod).rgb;
	vec2 brdf = texture(brdfLUT, vec2(NoV, roughness)).rg;
	vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

	return (kD * diffuse + specular) * ao;
}

void main()
{
	uint zIndex = uint(max(log2(LinearDepth(gl_FragCoord.z)) * cfUBO.sliceScaling + cfUBO.sliceBias, 0.0));
	//float linDepth = LinearDepth(gl_FragCoord.z);
	//float zIndexFloat = float(cfUBO.sliceCountZ) * log2(linDepth / cfUBO.cameraNear) / log2(cfUBO.cameraFar / cfUBO.cameraNear);
	//uint zIndex = uint(max(zIndexFloat, 0.0));

	vec2 tileSize = 
		vec2(cfUBO.screenSize.x / float(cfUBO.sliceCountX), 
			 cfUBO.screenSize.y / float(cfUBO.sliceCountY));

	uvec3 cluster = uvec3(
		gl_FragCoord.x / tileSize.x,
		gl_FragCoord.y / tileSize.y,
		zIndex);

	uint clusterIdx =
		cluster.x +
		cluster.y * cfUBO.sliceCountX +
		cluster.z * cfUBO.sliceCountX * cfUBO.sliceCountY;

	uint lightCount = lightCells.data[clusterIdx].count;
	uint lightIndexOffset = lightCells.data[clusterIdx].offset;

	// PBR
	vec4 albedo4 = texture(textureAlbedo, texCoord).rgba;

	if (albedo4.a < 0.5)
	{
		discard;
	}

	// Material properties
	vec3 albedo = pow(albedo4.rgb, vec3(2.2));
	vec3 emissive = texture(textureEmissive, texCoord).rgb;
	float metallic = texture(textureMetalness, texCoord).b;
	float roughness = texture(textureRoughness, texCoord).g;
	float alpha = AlphaDirectLighting(roughness);
	float ao = texture(textureAO, texCoord).r;

	vec3 tangentNormal = texture(textureNormal, texCoord).xyz * 2.0 - 1.0;

	// Input lighting data
	vec3 N = GetNormalFromMap(tangentNormal, worldPos, normal, texCoord);
	vec3 V = normalize(frameUBO.cameraPosition.xyz - worldPos);
	float NoV = max(dot(N, V), 0.0);

	// Calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
	// of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)  
	vec3 F0 = vec3(pc.baseReflectivity);
	F0 = mix(F0, albedo, metallic);

	// Reflectance equation
	vec3 Lo = vec3(0.0);

	//for (int i = 0; i < inLights.data.length(); ++i)
	for (int i = 0; i < lightCount; ++i)
	{
		//LightData light = inLights.data[i];
		uint lightIndex = lightIndices.data[i + lightIndexOffset];
		LightData light = inLights.data[lightIndex];

		Lo += Radiance(
			albedo,
			N,
			V,
			F0,
			metallic,
			roughness,
			alpha,
			NoV,
			light.position.xyz,
			light.color.xyz);
	}

	vec3 ambient = Ambient(
		albedo,
		F0,
		N,
		V,
		metallic,
		roughness,
		ao,
		NoV);

	vec3 color = ambient + emissive + Lo;

	fragColor = vec4(color, 1.0); // Default

	//float lightCountColor = float(lightCount) / 50.0;
	//fragColor = vec4(lightCountColor, lightCountColor, lightCountColor, 1.0);

	//float linDepth = LinearDepth(gl_FragCoord.z);
	//fragColor = vec4(linDepth, linDepth, linDepth, 1.0);

	//fragColor = vec4(color, 1.0) + vec4(DEBUG_COLORS_2[uint(mod(float(zIndex), 8.0))], 0.0);
	//float cLightCount = 1.0 - (float(lightCount) / 50.0);
	//fragColor = vec4(color, 1.0) + vec4(cLightCount * 0.1, 0.0, 0.0, 0.0);
}