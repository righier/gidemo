#version 460 core

in vec3 a_pos;
in vec2 a_uv;
in vec3 a_normal;
in vec4 a_posls;
// in vec3 a_xtan;
// in vec3 a_ytan;

uniform vec3 u_diffuse;
uniform float u_metal;
uniform float u_rough;
uniform float u_opacity;
uniform vec3 u_emission;

// uniform sampler2DArray u_tex;

uniform int u_voxelCount;
uniform bool u_aniso;

uniform vec3 u_cameraPos;

uniform sampler3D u_voxelTexture;
uniform sampler3D u_voxelTextureAniso[6];
const ivec3 DIR = ivec3(0, 2, 4);

uniform float u_offsetDist;
uniform float u_offsetPos;
uniform bool u_diffuseNoise;

uniform float u_time;

uniform vec3 u_lightPos;
uniform vec3 u_lightDir;
uniform vec3 u_lightColor;
uniform float u_lightInnerCos;
uniform float u_lightOuterCos;

uniform float u_shadowBias;

uniform sampler2D u_shadowMap;

layout(location = 0) out vec3 o_albedo;

float voxelSize = 1.0f / float(u_voxelCount);

const float PI = 3.14159265359;

vec3 ortho(vec3 v) {
  vec3 w = vec3(1,0,0);
  if (abs(dot(v, w)) > 0.999) w = vec3(0,0,1);
  return cross(v, w);
}

bool insideVoxel(vec3 pos) {
  return all(lessThanEqual(pos, vec3(1.0f))) && all(greaterThanEqual(pos, vec3(0.0f)));
}

vec3 toVoxel(vec3 pos) {
  return pos*(0.5)+0.5;  
}

float roughnessToAperture(float roughness){
  roughness = clamp(roughness, 0.0, 1.0);
  // return tan(0.0003474660443456835 + (roughness * (1.3331290497744692 - (roughness * 0.5040552688878546)))); // <= used in the 64k
  // return tan(acos(pow(0.244, 1.0 / (clamp(2.0 / max(1e-4, (roughness * roughness)) - 2.0, 4.0, 1024.0 * 16.0) + 1.0))));
  return clamp(tan((3.141592 * (0.5 * 0.75)) * roughness), 0.00174533102, 3.14159265359);
}


vec4 fetch(vec3 dir, vec3 pos, float lod) {
  pos = clamp(pos, vec3(0), vec3(1));
  if (u_aniso) {
    bvec3 sig = greaterThan(dir, vec3(0.0f));
    float l2 = max(0, lod - 1);
    vec4 cx = sig.x ? textureLod(u_voxelTextureAniso[1], pos, l2) : textureLod(u_voxelTextureAniso[0], pos, l2);
    vec4 cy = sig.y ? textureLod(u_voxelTextureAniso[3], pos, l2) : textureLod(u_voxelTextureAniso[2], pos, l2);
    vec4 cz = sig.z ? textureLod(u_voxelTextureAniso[5], pos, l2) : textureLod(u_voxelTextureAniso[4], pos, l2);

    vec3 sd = dir * dir; 
    vec4 c1 = sd.x * cx + sd.y * cy + sd.z * cz;

    if (lod < 1) {
      vec4 c2 = textureLod(u_voxelTexture, pos, 0);
      return mix(c2, c1, lod);
    } else {
      return c1;
    }
  } else {
    return textureLod(u_voxelTexture, pos, lod);
  }
}


float traceShadow(vec3 start, vec3 lightPos) {
  // start = toVoxel(start);
  // lightPos = toVoxel(lightPos);
  vec3 dir = lightPos - start;
  float lightDist = length(dir);
  dir /= lightDist;
  
  float acc = 0;

  float dist = 3*voxelSize;

  while (dist < lightDist && acc < 1.0) {
    vec3 pos = start + dir * dist;
    float l = pow(dist, 2);
    float s1 = 0.062 * textureLod(u_voxelTexture, toVoxel(pos), 1+0.75 * l).a;
    float s2 = 0.135 * textureLod(u_voxelTexture, toVoxel(pos), 4.5 * l).a;
    // float s = s1 + s2;
    float s = textureLod(u_voxelTexture, toVoxel(pos), 0.0).a;

    acc += (1-acc) * s;
    dist += 0.9 * voxelSize * (1 + 0.05 * l);
    // dist += voxelSize;

  }

  return pow(smoothstep(0, 1, acc * 1.4), 1.0f / 1.4);
  // return acc;
}

float shadow() {
  float bias = u_shadowBias;
  vec3 coords = a_posls.xyz / a_posls.w;
  coords = coords*0.5 + 0.5;
  float shadowDepth = texture(u_shadowMap, coords.xy).r;
  float pointDepth = coords.z;
  float shadowVal = pointDepth >= shadowDepth + bias ? 1.0 : 0.0;
  return shadowVal;
}

vec3 getDirectionWeights(vec3 direction){
  return direction * direction;
}


vec4 traceVoxelCone( vec3 from, vec3 direction, float aperture, float offset, float maxDistance){
  direction = normalize(direction);
  bvec3 negativeDirection = lessThan(direction, vec3(0.0));
  float doubledAperture = max(voxelSize, 2.0 * aperture),
  dist = offset;

  vec3 directionWeights = getDirectionWeights(direction),

  position = from + (dist * direction);
  vec4 accumulator = vec4(0.0);
  maxDistance = min(maxDistance, 1.41421356237);
  while((dist < maxDistance) && (accumulator.a < 1.0)){
    float diameter = max(voxelSize * 0.5, doubledAperture * dist),
    mipMapLevel = max(0.0, log2((diameter * float(u_voxelCount)) + 0.0));
    vec4 voxel = fetch(direction, position, mipMapLevel);
    accumulator += (1.0 - accumulator.w) * voxel;
    dist += max(diameter, voxelSize) * 0.5;
    position = from + (dist * direction);
  }
  return max(accumulator, vec4(0.0));
}

vec4 voxelJitterNoise(vec4 p4){
  p4 = fract(p4 * vec4(443.897, 441.423, 437.195, 444.129));
  p4 += dot(p4, p4.wzxy + vec4(19.19));
  return fract((p4.xxyz + p4.yzzw) * p4.zywx);
}

vec3 traceSpecular(float roughness) {
  vec4 acc = vec4(0);

  vec3 pos = toVoxel(a_pos);
  vec3 camPos = toVoxel(u_cameraPos);
  vec3 normal = a_normal;

  vec3 camDir = normalize(pos - camPos);
  vec3 dir = reflect(camDir, normal);


  const float coneAngle = radians(1.f);

  pos += normal * voxelSize * 1.f;//u_offsetPos;

  float maxdist = sqrt(3.f);
  float distOffset = voxelSize * 4.0; //u_offsetDist;

  float aperture = tan(coneAngle/2.0f);
  aperture = roughnessToAperture(roughness);

  return traceVoxelCone(pos, dir, aperture, distOffset, maxdist).xyz;

}



//
float NormalDistributionTrowbridgeReitz(float NdotH, float roughness) {
  float a = roughness*roughness;
  float a2 = a*a;
  float NdotH2 = NdotH*NdotH;
  float d = NdotH2 * (a2 - 1) + 1;
  return a2 / (PI * d*d);
}

float NormalDistributionGGX(float NdotH, float roughness) {
  float a = roughness*roughness;
  float NdotH2 = NdotH*NdotH;
  float TanNdotH2 = (1.0 - NdotH2) / NdotH2;
  float d = roughness / (NdotH2 * (a + TanNdotH2));
  return (1.0 / PI) * d*d;
}

// Geometrix Shadowing (Smith's method, Schlick Approximation)
// remaps roughness to (r + 1) / 2
float GeometrySchlickFixed(float NdotL, float NdotV, float roughness) {
  float r = (roughness + 1);
  float k = (r*r) / 8;
  vec2 gs = k + (1 - k) * vec2(NdotL, NdotV);
  return (NdotL * NdotV) / (gs.x * gs.y);
}

float GeometrySchlick(float NdotL, float NdotV, float roughness) {
  float k = (roughness*roughness) / 2;
  vec2 gs = k + (1 - k) * vec2(NdotL, NdotV);
  return (NdotL * NdotV) / (gs.x * gs.y);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
  return F0 + (1 - F0) * pow(1 - cosTheta, 5);
}

vec3 FresnelGaussian(float cosTheta, vec3 F0) {
  return F0 + (1.0f - F0) * exp2(cosTheta * (-5.55473*cosTheta - 6.98316));
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
} 



float random(vec3 v) {
  float a = dot(v, vec3(29.29384, 62.29384, 48.23478));
  a = sin(a);
  a = a * 293487.1273498;
  a = fract(a);
  return a;
}

vec3 random3(vec3 p3){
  p3 = fract(p3 * vec3(443.897, 441.423, 437.195));
  p3 += dot(p3, p3.zxy + vec3(31.31));
  return fract((p3.xyz + p3.yzx) * p3.zyx);
}

// vec4 traceVoxelCone( vec3 from, vec3 direction, float aperture, float offset, float maxDistance){
vec3 myTracing(vec3 from, vec3 dir, float apertureTan2) {
  float dist = voxelSize * 4;

  // float a2 = max(voxelSize, 2 * tan(apertureAngle / 2));
  float a2 = max(voxelSize, apertureTan2);

  vec4 color = vec4(0);
  while (dist < 1.733 && color.a < 1.0) {
    float diameter = max(voxelSize/2, dist * a2);
    float lod = log2(diameter / voxelSize);//* u_voxelCount);
    vec3 pos = from + dir * dist;
    // color = vec4(color.rgb, 1) * color.a + (1 - color.a) * fetch(dir, pos, lod);
    color += (1 - color.a) * fetch(dir, pos, lod);

    dist += diameter * 1.0;
  }
  // return vec3(1);

  return color.rgb;
}

vec3 myDiffuse() {
  vec3 color = vec3(0);

  vec3 pos = toVoxel(a_pos);
  vec3 normal = a_normal;
  pos += 2 * voxelSize * normal;

  #define CONES 5

  const vec3 cones[5] = vec3[5]( vec3(0,0,1), vec3(1,0,1), vec3(-1,0,1), vec3(0,1,1), vec3(0,-1,1));
  const float weight[5] = float[5](1,0.5,0.5,0.5,0.5);

  float apertureAngle = 30;
  float apertureTan2 = 2 * tan(radians(apertureAngle)/2);

  vec3 xtan, ytan;
  if (u_diffuseNoise) {
    xtan = normalize(random3(pos*u_time) * 2 + 1);
    if (abs(dot(xtan, normal)) > 0.999) {
      xtan = ortho(normal);
    }
    ytan = cross(normal, xtan);
    xtan = cross(normal, ytan);
  } else {
    xtan = ortho(normal);
    ytan = cross(normal, xtan);
    ytan = cross(xtan, normal);
  }
  mat3 tanSpace = mat3(xtan, ytan, normal); 

  for (int i = 0; i < CONES; i++) {
    vec3 dir = tanSpace * cones[i];
    color += weight[i] * myTracing(pos, normalize(dir), apertureTan2);
  }

  return color;
}

vec3 mySpecular(float roughness) {
  vec4 acc = vec4(0);

  vec3 pos = toVoxel(a_pos);
  vec3 camPos = toVoxel(u_cameraPos);
  vec3 normal = a_normal;

  vec3 camDir = normalize(pos - camPos);
  vec3 dir = reflect(camDir, normal);


  pos += normal * voxelSize * 1.f;

  float aperture = roughnessToAperture(roughness) * 2;

  return myTracing(pos, dir, aperture);

}

float myShadow() {
  vec3 normal = a_normal;
  vec3 from = toVoxel(a_pos);
  vec3 to = toVoxel(u_lightPos); 

  float aperture = 3;
  float a2 = tan(radians(aperture / 2)) * 2;

  vec3 dir = normalize(to - from);
  from += 4 * voxelSize * dir;
  from += 1 * voxelSize * normal;
  float maxDist = length(to - from);
  maxDist = min(maxDist, 1.7);

  // float dist = voxelSize * random(from);
  float dist = (2.5 + random(from)) * voxelSize;

  float acc = 0;
  while (acc < 1.0) {
    float diameter = max(voxelSize/2, dist * a2);
    if (dist > maxDist) break;
    float lod = max(0, log2(diameter / voxelSize + 1.0));//* u_voxelCount);
    vec3 pos = from + dir * dist;
    float cc = fetch(dir, pos, lod).a;
    acc += (1 - acc) * cc*cc;

    dist += diameter * 0.2;
  }

  return 1.0 - clamp(acc, 0.0, 1.0);
}

vec3 spotlight() {
  vec3 spotDir = u_lightDir;
  vec3 lightDir = normalize(u_lightPos - a_pos);
  float dist = abs(length(a_pos - u_lightPos));
  float angle = dot(spotDir, -lightDir);
  float intensity = smoothstep(u_lightOuterCos, u_lightInnerCos, angle);
  float attenuation = 1.0 / (1.0 + dist); 
  intensity *= attenuation;
  intensity *= max(0, dot(a_normal, lightDir));
  // intensity *= 1.0 - min(1.0f, traceShadow(a_pos, u_lightPos));
  // intensity *= 1.0 - min(1.0f, shadow());
  if (intensity > 0) {
    intensity *= myShadow();
    // intensity *= 1.0 - shadow();
  }
  return u_lightColor * intensity;
}




void main() {
  vec3 albedo = pow(u_diffuse, vec3(2.2)); //srgb fix
  vec3 emission = pow(u_emission, vec3(2.2));

  float roughness = u_rough;
  float metalness = u_metal;

  vec3 spot = spotlight();

  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo, metalness);

  vec3 N = normalize(a_normal);
  vec3 V = normalize(u_cameraPos - a_pos);
  vec3 L = normalize(u_lightPos - a_pos);
  vec3 H = normalize(V+L);

  float NdotH = max(dot(N, H), 0.0);
  float NdotV = max(dot(N, V), 0.0);
  float HdotV = max(dot(H, V), 0.0);
  float NdotL = max(dot(N, L), 0.0);

  float NDF = NormalDistributionTrowbridgeReitz(NdotH, roughness);   
  float G   = GeometrySchlickFixed(NdotL, NdotV, roughness);      
  vec3 F    = FresnelSchlick(HdotV, F0);

  vec3 nominator = NDF * G * F;
  float denominator = 4 * NdotV * NdotL + 0.001;

  vec3 spec = nominator / denominator;

  vec3 kS = F;
  vec3 kD = 1.0 - kS;
  kD *= 1.0 - metalness;

  vec3 directLight = spot * (kD * albedo / PI + spec);

  F = FresnelSchlickRoughness(NdotV, F0, roughness);
  kS = F;
  kD = 1.0 - kS;
  kD *= 1.0 - metalness;

  vec3 indirectDiffuse = myDiffuse();
  vec3 indirectSpecular = traceSpecular(roughness);
  // vec3 indirectSpecular = mySpecular(roughness);

  // indirectDiffuse *= 0.1;
  // indirectSpecular *= 0.3;

  // indirectDiffuse = pow(indirectDiffuse, vec3(2));
  // indirectDiffuse = vec3(0);


  vec3 ambientLight = (kD * indirectDiffuse * albedo + F * indirectSpecular); 




  vec3 indirectLight = vec3(0);

  vec3 color = vec3(0);

  color += directLight;
  color += ambientLight;
  color += emission;

  // color = directLight;
  // color = spot * albedo;

  // color = indirectDiffuse;

  // color = myDiffuse();
  // color = indirectDiffuseOld();

  // color += roughness * diffuseLight * albedo;
  // color += (1.0f - u_rough) * specular;
  // color += emission;

  // color = fresnelSchlick(dot(V,N), vec3(0));

  // color = indirectLight;

  color = color / (color + vec3(1.0f)); // tone mapping (Reinhard)

  color = pow(color, vec3(1.0/2.2)); // gamma correction

  o_albedo = color;
  // o_albedo = color;
}