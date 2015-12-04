#pragma once

#include <glm/glm.hpp>
#include "VoxelData.h"
#include <cmath>

using namespace glm;

/*
 * A very simple test voxel dataset. Just a monochrome box centered at the origin.
 */
class SinVoxelData : public VoxelData {
 public:
  SinVoxelData(int height, int off, int scale, Voxel v) :
    h_(height), o_(off), s_(scale), v_(v) { };
  virtual bool voxelAt(ivec3 position, Voxel* v) {
    if ( position.y <= o_ + h_ * sin((float)position.x / s_) * cos((float)position.z / s_)) {
      *v = v_;
      return true;
    } else {
      return false;
    }
  };
 private:
  int h_, o_, s_;
  Voxel v_; // Color
};
