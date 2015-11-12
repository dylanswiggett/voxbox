#pragma once

#include <glm/glm.hpp>
#include "VoxelData.h"

using namespace glm;

/*
 * A very simple test voxel dataset. Just a monochrome box centered at the origin.
 */
class BoxVoxelData : public VoxelData {
 public:
 BoxVoxelData(float w, float h, float d, vec3 c) : w_(w), h_(h), d_(d), c_(c) { };
  virtual Voxel voxelAt(vec3 position) {
    if ( position.x <= w_ && position.x >= -w_ &&
	 position.y <= h_ && position.y >= -h_ &&
	 position.z <= d_ && position.z >= -d_ ) {
      return Voxel{c_};
    } else {
      return Voxel{vec3()};
    }
  };
 private:
  float w_, h_, d_; // Dimensions
  vec3 c_; // Color
};