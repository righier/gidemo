#version 460 core

in vec3 a_pos;
// in vec2 a_uv;
// in vec3 a_normal;
// in vec3 a_xtan;
// in vec3 a_ytan;

uniform sampler3D voxelTexture;

uniform vec3 u_cameraPos;
uniform float u_voxelScale;
uniform int u_voxelCount;
uniform float u_voxelLod;

layout(location = 0) out vec3 o_albedo;

bool inside(vec3 v) {
  v = abs(v);
  return max(max(v.x, v.y), v.z) <= 1.0f;
}

void main() {
  if (gl_FragCoord.z <= 0) discard;

  const vec3 c = abs(u_cameraPos);

  vec3 start = (max(max(c.x,c.y),c.z) < u_voxelScale) ? u_cameraPos : a_pos;
  start /= u_voxelScale;
  vec3 dir = normalize(a_pos - u_cameraPos);

  vec4 color = vec4(0);

  float stepSize = 2.0f*pow(2, u_voxelLod) / (u_voxelCount*5.0f);
  // int stepCount = int(3.0f / stepSize);

  vec3 pos = start;

  // for (int step = 0; step < stepCount && color.a < 0.99f; step++) {
  int i = 0;
  int maxi = 1000;
  while (inside(pos) && color.a < 0.99f) {
    vec4 cc = textureLod(voxelTexture, pos*0.5 + 0.5, u_voxelLod);
    color += (1.0f - color.a) * cc;
    pos += dir*stepSize;
  }

  o_albedo = pow(color.rgb, vec3(1.0/2.2));
}