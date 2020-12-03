
in vec3 a_pos;
in vec2 a_uv;
in vec3 a_normal;
in vec4 a_posls;
in vec3 a_xtan;
in vec3 a_ytan;

layout(location = 0) out vec3 o_color;

uniform vec3 u_diffuse;
uniform vec3 u_emission;
uniform float u_emissionScale;
uniform float u_metal;
uniform float u_rough;

uniform bool u_useMaps;
uniform sampler2D u_diffuseMap;
uniform sampler2D u_bumpMap;
uniform sampler2D u_specMap;
uniform sampler2D u_emissionMap;

uniform vec3 u_cameraPos;

uniform vec3 u_lightPos;
uniform vec3 u_lightDir;
uniform vec3 u_lightColor;
uniform float u_lightInnerCos;
uniform float u_lightOuterCos;

uniform float u_shadowBias;

uniform sampler2D u_shadowMap;

uniform bool u_tracedShadows;
uniform float u_shadowAperture;

uniform int u_renderMode;
uniform int u_voxelCount;
float voxelSize = 1.0f / float(u_voxelCount);

#ifdef ANISO
bool u_aniso = true;
#else
bool u_aniso = false;
#endif

uniform sampler3D u_voxelTexture;
uniform sampler3D u_voxelTextureAniso[6];

uniform bool u_diffuseNoise;
uniform float u_restitution;

/* the time uniform is used to generate random values */
uniform float u_time;
float time = mod(u_time, 1.0);

const float PI = 3.14159265359;

/* wrapper to fetch both isotropic and anisotropic voxels */
vec4 fetch(vec3 dir, vec3 pos, float lod) {
  lod = max(0, lod);
  // pos = clamp(pos, vec3(0), vec3(1));
#ifdef ANISO
  /* we need to know for every axis the direction to sample from */
  bvec3 sig = greaterThan(dir, vec3(0.0f));
  float l2 = max(0, lod - 1); /* the first level is actually the 2nd */
  vec4 cx = sig.x ? textureLod(u_voxelTextureAniso[1], pos, l2) 
    : textureLod(u_voxelTextureAniso[0], pos, l2);
  vec4 cy = sig.y ? textureLod(u_voxelTextureAniso[3], pos, l2) 
    : textureLod(u_voxelTextureAniso[2], pos, l2);
  vec4 cz = sig.z ? textureLod(u_voxelTextureAniso[5], pos, l2) 
    : textureLod(u_voxelTextureAniso[4], pos, l2);
  vec3 sd = dir * dir; 
  vec4 c1 = sd.x * cx + sd.y * cy + sd.z * cz;

  /* if lod < 1 we need to interpolate between aniso and base texture */
  if (lod < 1) {
    vec4 c2 = textureLod(u_voxelTexture, pos, 0);
    return mix(c2, c1, lod);
  } else {
    return c1;
  }

#else
  return textureLod(u_voxelTexture, pos, lod);
#endif
}

/* shadowss with PCF */
float shadow() {
  vec4 coords = a_posls;
  float bias = u_shadowBias;
  coords = coords*0.5;
  coords += coords.w;
  float total = 0;
  const int nsamples = 4; /* the real sample number is nsample*nsample */
  float scale = 2 / 2048.0; /* how far is the farthest sample from the origin */
  for (int i = 0; i < nsamples; i++) {
    for (int j = 0; j < nsamples; j++) {
      vec2 offset = vec2(i, j) / nsamples * 2 - 1;
      offset *= scale;
      vec4 pos = coords + vec4(offset, 0, 0);

      float shadowDepth = textureProj(u_shadowMap, pos.xyw).r;
      float pointDepth = coords.z / coords.w; /*normalize depth */
      float val = float(pointDepth - bias >= shadowDepth);
      total += val;
    }
  }

  /* average of samples */
  total /= nsamples*nsamples*1.0;

  /* to prevent floating point errors */
  return clamp(total, 0, 1);
}

/* returns a vector that is orthogonal to v */
vec3 ortho(vec3 v) {
  vec3 w = abs(dot(v, normalize(vec3(.1,0,1)))) < 0.999 
    ? vec3(.1,0,1) : vec3(1,0,0);
  return normalize(cross(v, w));
}

/* returns true if pos is inside the unit cube */
bool insideVoxel(vec3 pos) {
  return all(lessThanEqual(pos, vec3(1.0f))) && all(greaterThanEqual(pos, vec3(0.0f)));
}

/* convert the range [-1, +1] to [0, +1] */
vec3 toVoxel(vec3 pos) {
  return pos*(0.5)+0.5;  
}

/* returns a pseudo random float using v as seed */
float random(vec3 v) {
  float a = dot(v, vec3(29.29384, 62.29384, 48.23478));
  a = sin(a);
  a = a * 293487.1273498;
  a = fract(a);
  return a;
}

/* returns a preudo random vec3 using v as seed */
vec3 random3(vec3 p3){
  p3 = fract(p3 * vec3(443.897, 441.423, 437.195));
  p3 += dot(p3, p3.zxy + vec3(31.31));
  return fract((p3.xyz + p3.yzx) * p3.zyx);
}

/* converts roughness to cone aperture for specular lighting */
/* source: https://rootserver.rosseaux.net/demoscene/prods/HowToUseVoxelConeTracingWithTwoBouncesForEverythingInsteadOfJustGlobalIllumination.pdf 
 * */
float roughnessToAperture(float roughness){
  roughness = clamp(roughness, 0.0, 1.0);
  return tan(0.0003474660443456835 + (roughness * (1.3331290497744692 - (roughness * 0.5040552688878546))));
}

/* normal ditribution function for PBR specular */
float NormalDistributionTrowbridgeReitz(float NdotH, float roughness) {
  float a2 = roughness*roughness;
  float d = NdotH*NdotH * (a2 - 1) + 1;
  return a2 / (PI * d*d);
}

/* Geometry shadowing function for PBR specular
 * (Smith's method, Schlick Approximation)
 * remaps roughness to (r + 1) / 2 */
float GeometrySchlickFixed(float NdotL, float NdotV, float roughness) {
  float r = (roughness + 1);
  float k = (r*r) / 8;
  vec2 gs = k + (1 - k) * vec2(NdotL, NdotV);
  return (NdotL * NdotV) / (gs.x * gs.y);
}

/* Fresnel function (Schlick Approximation) */
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
  return F0 + (1 - F0) * pow(1 - cosTheta, 5);
}

/* Fresnel function for IBL lighting */
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
  return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
} 

/* performs cone tracing starting at from and going into direction dir with
   cone aperture of apertureTan2, which is = 2*tan(aperture/2) */
/* smaller quality = higher quality*/
vec3 ConeTracing(vec3 from, vec3 dir, float apertureTan2, float offset, float quality) {
  vec4 color = vec4(0); /* starting color and opacity */
  float dist = (voxelSize) * offset; /* we need a different offset for different effects */

  /* makes sure the paerture is > 0 */
  float a2 = max((voxelSize), apertureTan2);

  /* keep looping until the cone is fully occluded */
  while (color.a < 1.0) {
    /* the idameter of the cone at distance dist */
    float diameter = max((voxelSize), dist * a2);
    float lod = log2(diameter * u_voxelCount);//* u_voxelCount);
    vec3 pos = from + dir * dist; /* updates current position */
    if (!insideVoxel(pos)) break; /* returns if outside the 3D texture */
    vec4 cc = fetch(dir, pos, min(lod, 10));
    color += (1 - color.a) * cc;

    dist += diameter * quality; /* march along the ray direction */
  }

  return color.rgb;
}

/* Casts some ray to approximate indirect diffuse lighting at the current pos */
vec3 TraceDiffuse(vec3 normal) {

  vec3 pos = toVoxel(a_pos); /* normalize pos to texture coordinates */
  pos += 2 * voxelSize * normal; /* offset to prevent self lighting */

 /* number of cones to use */
#define CONES 5
  const vec3 cones[9] = vec3[9]( 
      vec3(0,0,1), vec3(1,0,1), vec3(-1,0,1), vec3(0,1,1), vec3(0,-1,1), 
      vec3(1,1,.5), vec3(1,-1,.5), vec3(-1,1,.5), vec3(-1,-1,.5));

  float apertureAngle = 30; /* the aperture must be less than 45 degrees */
  float apertureTan2 = 2 * tan(radians(apertureAngle)/2);

  vec3 xtan, ytan;
  if (u_diffuseNoise) { /* random cone directions */
    xtan = normalize(random3(pos+time) * 2 + 1);
    xtan = normalize(random3(pos+time));
    if (abs(dot(xtan, normal)) > 0.999) {
      xtan = ortho(normal);
    }
    ytan = cross(normal, xtan);
    xtan = cross(normal, ytan);
  } else { /* not random */
    xtan = ortho(normal);
    ytan = cross(normal, xtan);
  }

  /* normal tangent space */
  mat3 tanSpace = mat3(xtan, ytan, normal); 

  vec3 color = vec3(0);
  for (int i = 0; i < CONES; i++) {
    vec3 dir = tanSpace * cones[i];
    color += ConeTracing(pos, normalize(dir), apertureTan2, 2, 0.7);
  }
  return color; /* the returned color is the sum of all casted cones */
}

/* casts one cone in the reflected view direction to approximate
 * specular indirect lighting */
vec3 TraceSpecular(vec3 normal, float roughness) {
  vec3 pos = toVoxel(a_pos) + normal * voxelSize * 4;
  vec3 camPos = toVoxel(u_cameraPos);
  vec3 camDir = normalize(pos - camPos); /* view direction */
  vec3 dir = reflect(camDir, normal); /* reflected view direction */
  /* rougher sourfaces have bigger aperture */
  float aperture = roughnessToAperture(roughness) * 2; 
  return ConeTracing(pos, dir, aperture, 0, 0.6);
}

/* Casts one cone in the direction of the light source to approximate
 * soft shadows */
float TraceShadow(vec3 normal) {
  vec3 from = toVoxel(a_pos);
  vec3 to = toVoxel(u_lightPos); 

  float aperture = u_shadowAperture;
  float a2 = tan(radians(aperture / 2)) * 2;

  vec3 dir = normalize(to - from);
  from += 4 * (voxelSize) * dir;
  from += 1.5 * (voxelSize) * normal;
  float maxDist = length(to - from);
  maxDist = min(maxDist, 1.7);

  /* add random noise to reduce banding*/
  float dist = (2.5 + random(from)) * (voxelSize);

  float acc = 0;
  while (acc < 1.0) {
    float diameter = max((voxelSize)/2, dist * a2);
    if (dist + diameter > maxDist) break;
    float lod = max(0, log2(diameter / (voxelSize) + 1.0));
    vec3 pos = from + dir * dist;
    float cc = fetch(dir, pos, lod).a;
    cc = cc*cc; /* makes the shadow less blocky */
    acc += (1 - acc) * cc;
    dist += diameter * 0.2; /* need high quality to prevent holes */
  }

  return 1.0 - clamp(acc, 0.0, 1.0);
}

/* calculates the light contribution coming from the spotlight 
 * taking into account the light color, the light attenuation, 
 * shadows and NdotL*/
vec3 spotlight(vec3 normal) {
  vec3 spotDir = u_lightDir;
  vec3 lightDir = normalize(u_lightPos - a_pos);
  float dist = abs(length(a_pos - u_lightPos));
  float angle = dot(spotDir, -lightDir);
  float intensity = smoothstep(u_lightOuterCos, u_lightInnerCos, angle);
  float attenuation = 1.0 / (1.0 + dist); 
  intensity *= attenuation;
  intensity *= max(0, dot(normal, lightDir));
  if (intensity > 0) {
    if (u_tracedShadows) {
      intensity *= TraceShadow(normal);
    } else {
      intensity *= 1.0 - shadow();
    }
  }
  return u_lightColor * intensity;
}

void main() {

  /* color correction gamma exponents */
  float gammaEditor = 2.2;
  float gammaTextures = 2.2;

  vec3 albedo = pow(u_diffuse, vec3(gammaEditor));
  vec3 emission = pow(u_emission, vec3(gammaEditor)); 
  emission *= u_emissionScale;

  float roughness = u_rough;
  float metalness = u_metal;

  vec3 normal = a_normal;

  if (u_useMaps) {
    albedo *= pow(texture(u_diffuseMap, a_uv).rgb, vec3(gammaTextures));
    emission *= pow(texture(u_emissionMap, a_uv).rgb, vec3(gammaTextures));

    /* roughness and metalness are stored in the same texture */
    vec3 specMap = texture(u_specMap, a_uv).rgb;
    roughness *= specMap.g;
    metalness *= specMap.b;

    /* normalize inside fragment shader because of 
     * interpolation between vertices*/
    mat3 tanSpace = mat3(normalize(a_xtan), normalize(a_ytan), normalize(a_normal));
    vec2 tn = texture(u_bumpMap, a_uv).rg * 2 - 1; /*remaps normal to [-1,+1] */
    normal = vec3(tn, sqrt(1 - dot(tn, tn))); /* calculates z component */
    normal = tanSpace * normal; /* normal vector corrected with normal map */
  }
  normal = normalize(normal);
  float roughnessLinear = roughness;
  roughness = roughness * roughness; /*better looking materials*/


  vec3 color = vec3(0);


  // direct lighting
  vec3 spot = spotlight(normal);

  /* reflectance at normal indidence */
  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo, metalness);

  vec3 N = normalize(normal);
  vec3 V = normalize(u_cameraPos - a_pos); /* view vector */
  vec3 L = normalize(u_lightPos - a_pos); /* light vector */
  vec3 H = normalize(V+L); /*halfway vector */

  float NdotH = max(dot(N, H), 0.0);
  float NdotV = abs(dot(N, V));
  float HdotV = max(dot(H, V), 0.0);
  float NdotL = max(dot(N, L), 0.0);

  float NDF = NormalDistributionTrowbridgeReitz(NdotH, roughness);   
  float G   = GeometrySchlickFixed(NdotL, NdotV, roughnessLinear);      
  vec3 F    = FresnelSchlick(HdotV, F0);

  vec3 nominator = NDF * G * F;
  float denominator = 4 * NdotV * NdotL + 0.001;
  vec3 spec = nominator / denominator;

  vec3 kS = F;
  vec3 kD = 1.0 - kS; 
  kD *= 1.0 - metalness; /* metals have no diffuse */

  /* compose direct lighting */
  vec3 directLight = spot * (kD * albedo / PI + spec);


  // indirect lighting
  F = FresnelSchlickRoughness(NdotV, F0, roughness);
  kS = F;
  kD = 1.0 - kS;
  kD *= 1.0 - metalness;

  /* add indirect diffuse component */
  vec3 indirectDiffuse = vec3(0.0);
  if (u_renderMode == 1 || u_renderMode == 3) {
    if (metalness < 1 || u_renderMode == 1) indirectDiffuse = TraceDiffuse(N);
    indirectDiffuse *= u_restitution;
    color = indirectDiffuse;
  }

  /* add indirect specular component */
  vec3 indirectSpecular = vec3(0.0);
  if (u_renderMode == 2 || u_renderMode == 3) {
    indirectSpecular = TraceSpecular(N, roughness);
    color = indirectSpecular;
  }


  // compose lighting
  if (u_renderMode == 0 || u_renderMode == 3) {
    vec3 ambientLight = (kD * indirectDiffuse * albedo + F * indirectSpecular); 

    color = directLight;
    color += ambientLight;
    color += emission;
  }

  // color correction
  color = color / (color + vec3(1.0f)); // tone mapping (Reinhard)
  color = pow(color, vec3(1.0/2.2)); // gamma correction
  o_color = color;
}
