#version 460 core

/*
Fragment shader for PBR+IBL, naive forward shading (non clustered)
*/

// Include files
#include <PBRHeader.frag>
#include <Hammersley.frag>
#include <ClusteredForward//ClusterForwardHeader.comp>

layout(location = 0) in vec3 worldPos;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PushConstantPBR
{
	float lightIntensity;
	float baseReflectivity;
	float maxReflectionLod;
	float lightFalloff; // Small --> slower falloff, Big --> faster falloff
	float albedoMultipler; // Show albedo color if the scene is too dark, default value should be zero
}
pc;

layout(set = 0, binding = 0) uniform CameraUBO
{
	mat4 projection;
	mat4 view;
	vec4 position;
}
camUBO;

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

// TODO Simplify these
layout(set = 0, binding = 3) readonly buffer Lights { LightData lights []; };
layout(set = 0, binding = 4) buffer LightCells { LightCell data []; } lightCells;
layout(set = 0, binding = 5) buffer LightIndices{ uint data []; } lightIndices;
layout(set = 0, binding = 6) readonly buffer Clusters { AABB data []; } clusters;

layout(set = 0, binding = 7) uniform sampler2D textureAlbedo;
layout(set = 0, binding = 8) uniform sampler2D textureNormal;
layout(set = 0, binding = 9) uniform sampler2D textureMetalness;
layout(set = 0, binding = 10) uniform sampler2D textureRoughness;
layout(set = 0, binding = 11) uniform sampler2D textureAO;
layout(set = 0, binding = 12) uniform sampler2D textureEmissive;

layout(set = 0, binding = 13) uniform samplerCube specularMap;
layout(set = 0, binding = 14) uniform samplerCube diffuseMap;
layout(set = 0, binding = 15) uniform sampler2D brdfLUT;

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

vec3 Radiance(
	vec3 albedo,
	vec3 N,
	vec3 V,
	vec3 F0,
	float metallic,
	float roughness,
	float alphaRoughness,
	float NoV,
	LightData light)
{
	vec3 Lo = vec3(0.0);

	vec3 L = normalize(light.position.xyz - worldPos); // Incident light vector
	vec3 H = normalize(V + L); // Halfway vector
	float NoH = max(dot(N, H), 0.0);
	float NoL = max(dot(N, L), 0.0);
	float HoV = max(dot(H, V), 0.0);
	float distance = length(light.position.xyz - worldPos);

	// Physically correct attenuation
	//float attenuation = 1.0 / (distance * distance);

	// Hacky attenuation for clustered forward
	float attenuation = max(1.0 - (distance / light.radius), 0.0) / pow(distance, pc.lightFalloff);

	// Also, several attenuation formulas are proposed by Nikita Lisitsa:
	// lisyarus.github.io/blog/graphics/2022/07/30/point-light-attenuation.html

	vec3 radiance = light.color.xyz * attenuation * pc.lightIntensity;

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

// stackoverflow.com/questions/51108596/linearize-depth
float LinearDepth(float z, float near, float far)
{
	return near * far / (far + z * (near - far));
}

void main()
{
	// Must use GLM_FORCE_DEPTH_ZERO_TO_ONE, or the code below will fail
	float linDepth = LinearDepth(gl_FragCoord.z, cfUBO.cameraNear, cfUBO.cameraFar);
	uint zIndex = uint(max(log2(linDepth) * cfUBO.sliceScaling + cfUBO.sliceBias, 0.0));

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
	float ao = texture(textureAO, texCoord).r;

	float alphaRoughness = AlphaDirectLighting(roughness);
	vec3 tangentNormal = texture(textureNormal, texCoord).xyz * 2.0 - 1.0;

	// Input lighting data
	vec3 N = GetNormalFromMap(tangentNormal, worldPos, normal, texCoord);
	vec3 V = normalize(camUBO.position.xyz - worldPos);
	float NoV = max(dot(N, V), 0.0);

	// Calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
	// of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)  
	vec3 F0 = vec3(pc.baseReflectivity);
	F0 = mix(F0, albedo, metallic);

	// A little bit hacky
	//vec3 Lo = vec3(0.0); // Original code
	vec3 Lo =  albedo * pc.albedoMultipler;

	for(int i = 0; i < lightCount; ++i)
	{
		uint lightIndex = lightIndices.data[i + lightIndexOffset];
		LightData light = lights[lightIndex];

		Lo += Radiance(
			albedo,
			N,
			V,
			F0,
			metallic,
			roughness,
			alphaRoughness,
			NoV,
			light);
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

	fragColor = vec4(color, 1.0);
}