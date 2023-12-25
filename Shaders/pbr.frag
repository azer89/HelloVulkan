#version 460 core

layout(location = 0) in vec3 worldPos;
layout(location = 1) in vec2 texCoord;
layout(location = 2) in vec3 normal;

layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform PerFrameUBO
{
	mat4 cameraProjection;
	mat4 cameraView;
	vec4 cameraPosition;
}
frameUBO;

layout(binding = 2) uniform sampler2D textureDiffuse;
layout(binding = 3) uniform sampler2D textureNormal;
layout(binding = 4) uniform sampler2D textureMetalness;
layout(binding = 5) uniform sampler2D textureRoughness;
layout(binding = 6) uniform sampler2D textureAO;
layout(binding = 7) uniform sampler2D textureEmissive;

layout(binding = 8) uniform samplerCube specularMap;
layout(binding = 9) uniform samplerCube diffuseMap;
layout(binding = 10) uniform sampler2D brdfLUT;

const int NUM_LIGHTS = 4;

// Hardcoded lights
vec3 lightPositions[NUM_LIGHTS] = 
{ 
	vec3(-1.5, 0.7,  1.5),
	vec3( 1.5, 0.7,  1.5),
	vec3(-1.5, 0.7, -1.5),
	vec3( 1.5, 0.7, -1.5)
};
vec3 lightColors[NUM_LIGHTS] = 
{
	vec3(15.7, 10.0, 10.0),
	vec3(15.7, 10.0, 10.0),
	vec3(15.7, 10.0, 10.0),
	vec3(15.7, 10.0, 10.0)
};

#include <pbr_include.frag>

void main()
{
	vec4 albedo4 = texture(textureDiffuse, texCoord).rgba;

	if (albedo4.a < 0.5)
	{
		discard;
	}

	// Material properties
	vec3 albedo = pow(albedo4.rgb, vec3(2.2)); ;
	vec3 emissive = texture(textureEmissive, texCoord).rgb;
	float metallic = texture(textureMetalness, texCoord).b;
	float roughness = texture(textureRoughness, texCoord).g;
	float ao = texture(textureAO, texCoord).r;

	vec3 tangentNormal = texture(textureNormal, texCoord).xyz * 2.0 - 1.0;

	// Input lighting data
	vec3 N = GetNormalFromMap(tangentNormal, worldPos, normal, texCoord);
	vec3 V = normalize(frameUBO.cameraPosition.xyz - worldPos);
	vec3 R = reflect(-V, N);

	// Calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
	// of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)  
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	// Reflectance equation
	vec3 Lo = vec3(0.0);
	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		// Calculate per-light radiance
		vec3 L = normalize(lightPositions[i] - worldPos);
		vec3 H = normalize(V + L);
		float distance = length(lightPositions[i] - worldPos);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = lightColors[i] * attenuation;

		// Cook-Torrance BRDF
		float NDF = DistributionGGX(N, H, roughness);
		float G = GeometrySmith(N, V, L, roughness);
		vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

		vec3 numerator = NDF * G * F;
		float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
		vec3 specular = numerator / denominator;

		// kS is equal to Fresnel
		vec3 kS = F;
		// for energy conservation, the diffuse and specular light can't
		// be above 1.0 (unless the surface emits light); to preserve this
		// relationship the diffuse component (kD) should equal 1.0 - kS.
		vec3 kD = vec3(1.0) - kS;
		// multiply kD by the inverse metalness such that only non-metals 
		// have diffuse lighting, or a linear blend if partly metal (pure metals
		// have no diffuse light).
		kD *= 1.0 - metallic;

		// Scale light by NdotL
		float NdotL = max(dot(N, L), 0.0);

		// Add to outgoing radiance Lo
		Lo += (kD * albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
	}

	// Ambient lighting (we now use IBL as the ambient term)
	vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

	vec3 kS = F;
	vec3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;

	vec3 irradiance = texture(diffuseMap, N).rgb;
	vec3 diffuse = irradiance * albedo;

	// Sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
	const float MAX_REFLECTION_LOD = 4.0;
	vec3 prefilteredColor = textureLod(specularMap, R, roughness * MAX_REFLECTION_LOD).rgb;
	vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
	vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

	vec3 ambient = (kD * diffuse + specular) * ao;

	vec3 color = ambient + emissive + Lo;

	// HDR tonemapping
	color = color / (color + vec3(1.0));

	// Gamma correction
	color = pow(color, vec3(1.0 / 2.2));

	fragColor = vec4(color, 1.0);
}