#version 330 core
in vec2 tex_coords;
in vec3 normal;
in vec3 frag_pos;
in vec4 frag_pos_light;

out vec4 frag_color;

uniform sampler2D diffuse;
uniform sampler2D normmap;
uniform sampler2D shadow_map;

uniform vec3 point_pos;
uniform vec3 dir_dir;
uniform vec3 view_pos;
uniform vec3 spot_pos;
uniform vec3 spot_dir;

/* Ambient light */
float amb_str = 0.3;

vec3 amb_color = vec3(1,1,1);

float spec_str = 0.5;

/* point */
float constant = 1;
float linear = 0.09;
float quad = 0.032;

/* spotlight */
float cutoff = 12.5; /* cutoff in radians */
float outer_cutoff = 17.5;

vec3 norm = vec3(0,0,0);
vec3 view = vec3(0,0,0);

float light_str(vec3 dir)
{
  float d = max(dot(norm, dir), 0.0);
  vec3 refl = reflect(-dir, norm);
  float s = pow(max(dot(view,refl), 0.0), 32);

  return s+d;
}

float shadow_calc(vec4 fg)
{
  vec3 pc = fg.xyz / fg.w;
  pc = pc * 0.5 + 0.5;
  
  if (pc.z > 1.0)
    return 0.0;

  float closest_depth = texture(shadow_map, pc.xy).r;
  float cur_depth = pc.z;

  vec3 light_dir = normalize(vec3(4,100,20) - frag_pos); /* light pos */
  float bias = max(0.05 * (1 - dot(norm, light_dir)), 0.005);

  return cur_depth - bias > closest_depth ? 1.0 : 0.0;

  float s;
  vec2 texel_size = 1 / textureSize(shadow_map, 0);

  for (int x = -1; x <= 1; ++x) {
    for (int y = -1; y <= 1; ++y) {
      float pcf_depth = texture(shadow_map, pc.xy + vec2(x,y) * texel_size).r;
      s += cur_depth - bias > pcf_depth ? 1.0 : 0.0;
    }
  }

  s /= 9.0;

  return s;
}

void main() {
  norm = normalize(normal);
  view = normalize(view_pos - frag_pos);

  float point_amt = light_str(normalize(point_pos-frag_pos));
  float dist = length(point_pos - frag_pos);
  float atten = 1.0 / (constant + linear * dist + quad * (dist*dist));
  point_amt *= atten;
  
  float dir_amt = light_str(normalize(-dir_dir));

  vec3 spot_dir = normalize(spot_pos - frag_pos);
  float theta = dot(spot_dir, normalize(-spot_dir));
  float spot_amt = 0;

  float epsilon = cutoff - outer_cutoff;

  if (theta > cutoff) {
    float intensity = clamp((theta - outer_cutoff)/epsilon,0.0,1.0);
    spot_amt = light_str(spot_dir) * intensity;
  }

  vec4 mm = texture(diffuse,tex_coords);
  float shadow = shadow_calc(frag_pos_light);
  vec3 res = mm.rgb * (amb_str + (point_amt + dir_amt) * (1 - shadow));
  frag_color = vec4(res, mm.a);
}
