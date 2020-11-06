
in vec3 a_pos;
in vec2 a_uv;
in vec3 a_normal;

uniform vec3 u_diffuse;

layout(location = 0) out vec3 o_albedo;

void main() {
	o_albedo = u_diffuse;
}

