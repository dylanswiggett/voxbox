#pragma once

#include <glm/glm.hpp>
#include "VoxelData.h"

using namespace glm;

/*
 * A very simple test voxel dataset. Just a monochrome box centered at the origin.
 */
class BoxVoxelData : public VoxelData {
 public:
 BoxVoxelData(vec3 corner, vec3 dim , vec3 c) : c1_(corner), c2_(corner + dim), c_(c) { };
  virtual bool voxelAt(vec3 position, Voxel* v) {
    if ( position.x < c2_.x && position.x >= c1_.x &&
	 position.y < c2_.y && position.y >= c1_.y &&
	 position.z < c2_.z && position.z >= c1_.z ) {
      v->color = c_;
      return true;
    } else {
      return false;
    }
  };
 private:
  vec3 c1_, c2_; // Dimensions
  vec3 c_; // Color
};
