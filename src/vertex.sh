#version 330

in vec3 vertex;
in vec3 normal;
in vec3 color;

uniform float color_factor;
uniform vec3 light_position;
uniform mat4 Projection;
uniform mat4 View;
uniform mat4 Model;

out vec3 light_vector;
out vec3 normal_vector;
out vec3 halfway_vector;
out vec3 color_out;
out float c_factor;

void main() {
	vec4 v = vec4(vertex,            1.0);
	vec4 n = vec4(normalize(normal), 1.0);
	vec4 l = vec4(light_position,    1.0);

	v = View * Model * v;
	n = inverse(transpose(View * Model)) * n;
	l = View * l;

	light_vector = normalize(l.xyz - v.xyz);
	normal_vector = normalize(n.xyz);
	halfway_vector = light_vector + normalize(-v.xyz);

	gl_Position = Projection * v;

	color_out = color;
	
	c_factor = color_factor;
}