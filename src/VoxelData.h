#pragma once

#include <iostream>
#include <glm/glm.hpp>

using namespace glm;
using namespace std;

struct Voxel {
  Voxel(vec3 c, uint8_t e, uint8_t d) {
    color = c;
    emit = e;
    diffuse = d;
  };
  Voxel() {
    color = vec3();
    emit = diffuse = 0;
  };
  vec3 color;
  uint8_t emit;
  uint8_t diffuse;
};

/*
ostream &operator<<(ostream &os, Voxel const &vox)
{
  return os << "{Voxel:c=(" << vox.color.r << "," <<
    vox.color.g << "," << vox.color.b << ")}";
}
*/

class VoxelData {
 public:
  virtual bool voxelAt(ivec3 position, Voxel *v) = 0;
};
