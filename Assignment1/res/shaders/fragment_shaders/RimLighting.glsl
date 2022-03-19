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
// Create a uniform for the material
uniform Material u_Material;


void main() {
	vec3 normal = normalize(inNormal);
	float rimIntensity = dot(eye, normal);

	float specPower = texture(u_Material.Specular, inUV).r;
	vec3 eye = normalize(u_CamPos.xyz - inWorldPos);
	vec4 texColor = texture(u_Material.Diffuse, inUV);
	vec3 lightColor = vec3(1.0, 1.0, 1.0);

	float strength = texture(u_Material.Specular, inUV).r;
	
	vec3 viewDirection = normalize(u_CamPos.xyz - inWorldPos);
	vec3 reflectDirection = reflect(-viewDirection, normal); //to point towards player
	
	float spec = pow(max(dot(viewDirection, reflectDirection), 0.0), 16);

	vec3 result = spec * inColor * texColor.rgb * lightColor;

	frag_color = vec4(result * rimIntensity, 1.0);
}