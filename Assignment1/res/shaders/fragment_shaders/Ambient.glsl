#version 430

#include "../fragments/fs_common_inputs.glsl"
#include "../fragments/multiple_point_lights.glsl"
#include "../fragments/frame_uniforms.glsl"
#include "../fragments/color_correction.glsl"

// output to color buffer
layout(location = 0) out vec4 frag_color;

struct Material {
	sampler2D Diffuse;
	float Shininess;
	float strength
};

uniform Material u_Material;

void main() {
	float strength = 0.5;
	vec4 texColor = texture(u_Material.Diffuse, inUV);
	vec3 lightColor = vec3(1.0, 1.0, 1.0);

	// combine for the final result
	vec3 result = texColor.rgb * lightColor * inColor * strength; 

	frag_color = vec4((result), 1.0);
}