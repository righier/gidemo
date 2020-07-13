#version 460 core

in vec3 a_pos;
in vec2 a_uv;
in vec3 a_normal;
// in vec3 a_xtan;
// in vec3 a_ytan;

uniform vec3 u_diffuse;
uniform float u_metal;
uniform float u_rough;
uniform float u_opacity;
uniform vec3 u_emission;

// uniform sampler2DArray u_tex;

struct PointLight {
	vec3 pos;
	vec3 color;
	vec3 atten;
};

struct DirLight {
	vec3 dir;
	vec3 color;
};

#define MAX_NPOINTLIGHTS 4
uniform int u_nPointLights;
uniform vec3 u_pointLights[MAX_NPOINTLIGHTS*3];

uniform sampler3D u_voxelTexture;

// #define MAX_NDIRLIGHTS 4
// uniform int u_nDirLights;
// uniform DirLight u_dirLights[MAX_NDIRLIGHTS];

layout(location = 0) out vec3 o_albedo;

void main() {
  // o_albedo = texture(u_tex, vec3(a_uv, 0)).rgb;
  // o_metal_rough_ao.r = texture(u_tex, vec3(a_uv, 2)).r;
  // o_metal_rough_ao.g = texture(u_tex, vec3(a_uv, 3)).r;
  // o_metal_rough_ao.b = 1.0f;

  // mat3 tangent_matrix = mat3(a_xtan, a_ytan, a_normal);
  // vec3 t_normal = texture(u_tex, vec3(a_uv, 1)).xyz * 2 - 1;
  // o_normal = tangent_matrix * t_normal;

  vec3 ambientColor = vec3(0);

  vec3 totalLight = ambientColor;
  for (int i = 0; i < u_nPointLights*3; i += 3) {

  	vec3 lightPos = u_pointLights[i];
  	vec3 lightColor = u_pointLights[i+1];
  	vec3 lightAtten = u_pointLights[i+2];

  	vec3 lightDir = lightPos - a_pos;
  	float dist = length(lightDir);
  	lightDir = lightDir / dist;

  	float s = 1.0 / (lightAtten.x + lightAtten.y*dist + lightAtten.z*dist*dist);
  	float t = max(0, dot(a_normal, lightDir));
  	totalLight += lightColor * s * t;
  }

  vec3 color = totalLight * u_diffuse;
  o_albedo = pow(color, vec3(1.0/2.2));
}