#version 430 core

layout(location = 0) out vec3 color;

struct voxel_data {
  uint color; // (r g b emittance)
  uint lighting; // (r g b diffuse)
  uint metadata; // (numrays neighbors flags flags)
};

uniform sampler3D voxels;

uniform ivec2 wsize;

uniform vec3 corner, dim;
uniform ivec3 nvoxels;

vec3 c1, c2;

layout (std430, binding=2) buffer voxel_buf {
  voxel_data vdata[];
};

float intersection(vec3 pos, vec3 dir) {
  float tmin = -100000, tmax = 100000;

  float tx1 = (c1.x - pos.x)/dir.x;
  float tx2 = (c2.x - pos.x)/dir.x;

  tmin = max(tmin, min(tx1, tx2));
  tmax = min(tmax, max(tx1, tx2));

  float ty1 = (c1.y - pos.y)/dir.y;
  float ty2 = (c2.y - pos.y)/dir.y;

  tmin = max(tmin, min(ty1, ty2));
  tmax = min(tmax, max(ty1, ty2));

  float tz1 = (c1.z - pos.z)/dir.z;
  float tz2 = (c2.z - pos.z)/dir.z;

  tmin = max(tmin, min(tz1, tz2));
  tmax = min(tmax, max(tz1, tz2));

  if (tmax >= tmin) {
    return tmin;
  } else {
    return 0.0;
  }
}

void main() {
  float mindim = min(wsize.x, wsize.y);
  vec2 pos = (2 * (gl_FragCoord.xy - wsize / 2) / mindim);
  c1 = corner;
  c2 = corner + dim;
  
  vec3 cameradir = -normalize(dim);
  vec3 right = normalize(cross(cameradir, vec3(0,1,0)));
  vec3 up = cross(right, cameradir);
  // TODO: Figure out why 25 is a good parameter here...
  vec3 camerapos = corner + dim + (right * pos.x + up * pos.y) * 25;

  float t = intersection(camerapos, cameradir);
  if (t == 0) {
    color = vec3(.1, .1, .2);
    return;
  }

  camerapos += cameradir * t;
  color = camerapos;
}
