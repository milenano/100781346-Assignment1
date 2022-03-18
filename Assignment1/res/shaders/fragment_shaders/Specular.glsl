#version 430

#include "../fragments/fs_common_inputs.glsl"
#include "../fragments/multiple_point_lights.glsl"
#include "../fragments/frame_uniforms.glsl"
#include "../fragments/color_correction.glsl"

layout(location = 0) out vec4 frag_color;

struct Material {
	sampler2D Diffuse;
	sampler2D Specular;
	float Shininess;
};

uniform Material u_Material;

void main() {
	vec3 normal = normalize(inNormal);
	vec4 texColor = texture(u_Material.Diffuse, inUV);

	float strength = texture(u_Material.Specular, inUV).r;
	vec3 viewDirection = normalize(u_CamPos.xyz - inWorldPos);
	vec3 reflectDirection = reflect(-viewDirection, normal);
	vec3 reflected = SampleEnvironmentMap(reflectDirection);


	vec3 result =  CalcAllLightContribution(inWorldPos, normal, u_CamPos.xyz, strength)  * inColor * texColor.rgb;

	frag_color = vec4(ColorCorrect(mix(result, reflected, strength)), texColor.a);
}