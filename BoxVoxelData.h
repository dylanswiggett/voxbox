#pragma once

#include <glm/glm.hpp>
#include "VoxelData.h"

using namespace glm;

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
  float w_, h_, d_;
  vec3 c_;
};
