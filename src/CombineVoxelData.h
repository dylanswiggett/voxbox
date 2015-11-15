#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "VoxelData.h"

using namespace glm;
using namespace std;

/*
 * A very simple test voxel dataset. Just a monochrome box centered at the origin.
 */
class CombineVoxelData : public VoxelData {
 public:
  CombineVoxelData() { };
  virtual bool voxelAt(vec3 position, Voxel* v) {
    for (auto vox : voxels)
      if (vox->voxelAt(position, v))
	return true;
    return false;
  };
  virtual void addVoxels(VoxelData *v) {
    voxels.push_back(v);
  }
 private:
  std::vector<VoxelData *> voxels;
};
