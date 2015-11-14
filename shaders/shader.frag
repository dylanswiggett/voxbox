#version 430 core

layout(location = 0) out vec3 color;

struct voxel_data {
  uint color; // (r g b emittance)
  uint lighting; // (r g b diffuse)
  uint metadata; // (numrays neighbors flags flags)
};

uniform usampler3D voxels;

uniform ivec2 wsize;

uniform vec3 corner, dim;
uniform ivec3 nvoxels;

vec3 c1, c2;

layout (std430, binding=2) buffer voxel_buf {
  voxel_data vdata[];
};

float rand(vec2 co){
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

ivec3 raymarch(vec3 pos, vec3 dir) {
  vec3 offset = pos - corner;
  vec3 voxelScale = (dim - corner) / nvoxels;
  int maxtest = 1000; // Just to avoid infinite loops :)
  while (maxtest > 0) {
    // TODO: Optimize these divisions!
    maxtest -= 1;
    ivec3 curVoxel = ivec3(floor(offset / voxelScale));

    // Are we in the box?
    if (curVoxel.x < 0 || curVoxel.x >= nvoxels.x ||
	curVoxel.y < 0 || curVoxel.y >= nvoxels.y ||
	curVoxel.z < 0 || curVoxel.z >= nvoxels.z)
      return ivec3(-1,-1,-1);

    // Are we in a voxel?
    uint vloc = texture(voxels, vec3(curVoxel) / nvoxels).r;
    if (vloc != 0)
      return curVoxel;

    // Find the next voxel!
    vec3 voxelOffset = offset - voxelScale * curVoxel;
    vec3 tsteps = max(voxelOffset / dir, (voxelOffset - voxelScale) / dir);
    float tstep = min(tsteps.x, min(tsteps.y, tsteps.z));
    offset += dir * (tstep + .01);
  }
  return ivec3(-1, -1, -1);
}

float intersection(vec3 pos, vec3 dir) {
  // TODO: Since the view is isometric,
  //       this can be much more efficient!
  vec3 t1 = (c1 - pos) / dir;
  vec3 t2 = (c2 - pos) / dir;
  vec3 tmins = min(t1, t2);
  vec3 tmaxs = max(t1, t2);
  float tmin = max(tmins.x, max(tmins.y, tmins.z));
  float tmax = min(tmaxs.x, min(tmaxs.y, tmaxs.z));
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

  camerapos += cameradir * (t + .1);

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
