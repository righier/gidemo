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
  vec3 color = u_diffuse;
  vec4 voxelDiffuse = textureLod(u_voxelTexture, a_pos/u_voxelScale)
  color = 

  o_albedo = pow(color, vec3(1.0/2.2));
}