
in vec3 a_pos;
in vec2 a_uv;

uniform vec3 u_emission;
uniform float u_opacity;

uniform sampler2D u_emissionMap;

layout(location = 0) out vec3 o_color;

void main() {
	float opacity = texture(u_emissionMap, a_uv).r * u_opacity;
	vec3 color = u_emission * opacity;
	// color = color / (color + vec3(1.0f)); // tone mapping (Reinhard)
  	// color = pow(color, vec3(1.0/2.2)); // gamma correction
  	o_color = color;
}