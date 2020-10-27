#version 460 core

in vec3 a_pos;
// in vec2 a_uv;
// in vec3 a_normal;
// in vec3 a_xtan;
// in vec3 a_ytan;


uniform vec3 u_cameraPos;
uniform int u_voxelCount;
uniform float u_voxelLod;
uniform float u_quality;
uniform bool u_aniso;
uniform int u_voxelIndex;

float voxelSize = 1.0f / u_voxelCount;

uniform sampler3D u_voxelTexture;
uniform sampler3D u_voxelTextureAniso[6];

layout(location = 0) out vec3 o_albedo;

bool inside(vec3 v) {
  v = abs(v);
  return max(max(v.x, v.y), v.z) <= 1.0f;
}

bool insideVoxel(vec3 pos) {
  return all(lessThanEqual(pos, vec3(1.0f))) && all(greaterThanEqual(pos, vec3(0.0f)));
}

vec3 toVoxel(vec3 pos) {
  return pos*(0.5)+0.5;  
}

float getMaxComponent(vec3 p) {
  vec3 q = abs(p);
  return max(max(q.x, q.y), q.z);
}

void main() {
  // if (gl_FragCoord.z <= 0) return;

  vec3 camv = toVoxel(u_cameraPos);
  vec3 posv = toVoxel(a_pos);
  vec3 dir = normalize(posv - camv);

  float hi = length(posv - camv);
  float lo = 0.0f;
  for (int i = 0; i < 10; i++) {
    float mid = (hi + lo) * 0.5f;
    if (insideVoxel(camv + dir*mid)) hi = mid;
    else lo = mid;
  }

  vec3 start = camv + dir * hi;

  vec4 color = vec4(0);

  float stepSize = pow(2, u_voxelLod) / (u_voxelCount*u_quality);

  vec3 pos = start;

  int lod = int(u_voxelLod);
  vec3 dim = vec3(float(u_voxelCount)/pow(2, float(lod)));
  int i = 0;
  int maxi = 1000;
  while (insideVoxel(pos) && color.a < 0.99f) {

    // vec3 pp = vec3(ivec3(pos*dim)) / dim + voxelSize*(1 << lod) / 2;
    // vec4 cc = textureLod(u_voxelTexture[u_voxelIndex], pp, u_voxelLod);
    vec4 cc;
    if (lod > 0 && u_aniso) {
      cc = texelFetch(u_voxelTextureAniso[u_voxelIndex], ivec3(pos*dim), lod-1);
    } else {
      cc = texelFetch(u_voxelTexture, ivec3(pos*dim), lod);
    }
    // vec4 cc = fetch(dir, pos, u_voxelLod);
    color += (1.0f - color.a) * vec4(cc.xyz, 1.0f) * cc.a;
    pos += dir*stepSize;
  }

  // if (color.a <= 0.01f) discard;



  // color = color / (color + vec4(1.0f)); // tone mapping (Reinhard)

  o_albedo = pow(color.rgb, vec3(1.0/2.2));
}