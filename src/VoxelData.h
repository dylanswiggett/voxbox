#pragma once

#include <iostream>
#include <glm/glm.hpp>

using namespace glm;
using namespace std;

typedef struct Voxel {
  vec3 color;
} Voxel;

ostream &operator<<(ostream &os, Voxel const &vox)
{
  return os << "{Voxel:c=(" << vox.color.r << "," <<
    vox.color.g << "," << vox.color.b << ")}";
}

class VoxelData {
 public:
  virtual Voxel voxelAt(vec3 position) = 0;
};
