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
	// Normalize our input normal
	vec3 normal = normalize(inNormal);
	vec3 lightDirection = (0.0, 0.0, -1.0);
	vec3 lightColor = (1.0, 1.0, 1.0);

	vec3 result = max(dot(normal.xyz, lightDirection), 0.0) * lightColor;

	frag_color = vec4(result, 1.0);
}