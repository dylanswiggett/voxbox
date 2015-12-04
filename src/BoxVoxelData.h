#pragma once

#include <glm/glm.hpp>
#include "VoxelData.h"

using namespace glm;

/*
 * A very simple test voxel dataset. Just a monochrome box centered at the origin.
 */
class BoxVoxelData : public VoxelData {
 public:
 BoxVoxelData(ivec3 corner, ivec3 dim , Voxel v) : c1_(corner), c2_(corner + dim), v_(v) { };
  virtual bool voxelAt(ivec3 position, Voxel* v) {
    if ( position.x < c2_.x && position.x >= c1_.x &&
	 position.y < c2_.y && position.y >= c1_.y &&
	 position.z < c2_.z && position.z >= c1_.z ) {
      *v = v_;
      return true;
    } else {
      return false;
    }
  };
 private:
  ivec3 c1_, c2_; // Dimensions
  Voxel v_; // Color
};
