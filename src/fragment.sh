#version 330

in vec3 light_vector;
in vec3 normal_vector;
in vec3 halfway_vector;
in vec3 color_out;
in float c_factor;
out vec4 color;

void main (void) {
	vec3 normal_vector1  = normalize(normal_vector);
	vec3 light_vector1   = normalize(light_vector);
	vec3 halfway_vector1 = normalize(halfway_vector);

	vec4 colord = vec4(color_out, 1.0);
	
	vec4 emissive_color = vec4(1.0, 1.0, 1.0, 1.0);
	vec4 ambient_color  = vec4(1.0, 1.0, 1.0, 1.0);
	vec4 diffuse_color  = vec4(1.0, 1.0, 1.0, 1.0);
	vec4 specular_color = vec4(1.0, 1.0, 1.0, 1.0);

	float d = dot(normal_vector1, light_vector1);
	bool facing = d > 0.0;

	float emissive_contribution = 0.15;
	float ambient_contribution  = 0.35;
	float diffuse_contribution  = 0.50;
	float specular_contribution = 0.00;

	color = emissive_color * emissive_contribution +
		ambient_color  * ambient_contribution  * colord +
		diffuse_color  * diffuse_contribution  * colord *     max(dot(normal_vector1,   light_vector1), 0) +
		specular_color * specular_contribution * colord * pow(max(dot(normal_vector1, halfway_vector1), 0), 128);

	color = color * c_factor;
	color.a = 1.0;
}