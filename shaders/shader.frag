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

ivec3 raymarch(vec3 pos, vec3 dir) {
  vec3 offset = pos - corner;
  vec3 voxelScale = (dim - corner) / nvoxels;
  int maxtest = 1000; // Just to avoid infinite loops :)
  while (maxtest > 0) {
    maxtest -= 1;
    ivec3 curVoxel = ivec3(offset / voxelScale);
    /*
    if (curVoxel.x % 2 == 0)
      return ivec3(-1, -1, -1);
    else
      return ivec3(0, 0, 0);
    */
    if (curVoxel.x < 0 || curVoxel.x >= nvoxels.x ||
	curVoxel.y < 0 || curVoxel.y >= nvoxels.y ||
	curVoxel.z < 0 || curVoxel.z >= nvoxels.z)
      return ivec3(-1,-1,-1);
    vec4 vloc = texelFetch(voxels, curVoxel, 0);
    //ivec4 vloc = ivec4(texture(voxels, vec3(curVoxel) / nvoxels) * 255);
    //ivec4 vloc = ivec4(texture(voxels, vec3(.8,.8,.8)) * 255);
    if (vloc.r + vloc.g + vloc.b + vloc.a != 0)
      return curVoxel;
    /*
    int loc = int(vloc.a + 255 * (vloc.b + 255 * (vloc.g + 255 * vloc.r)));
    if (loc != 0)
      return curVoxel;
    */

    vec3 voxelOffset = offset - voxelScale * curVoxel;
    offset += dir * .1;
  }
  return ivec3(-1, -1, -1);
}

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

  camerapos += cameradir * (t + .01);

  ivec3 voxel = raymarch(camerapos, cameradir);
  if (voxel.x < 0) {
    color = vec3(.2, .2, .4);
    return;
  }

  color = vec3(voxel) / nvoxels;
  return;
  
  ivec4 vloc = ivec4(texture(voxels, vec3(voxel) / nvoxels) * 255);
  int loc = vloc.a + 255 * (vloc.b + 255 * (vloc.g + 255 * vloc.r));
  // TODO: Use loc.

  color = vec3(1,1,1);
}
