// Non-clustered version
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

	// Hacky attenuation
	float attenuation = 1.0 / pow(distance, pc.lightFalloff);

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