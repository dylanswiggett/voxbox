#include <stack>

#include "VoxelOctree.h"

#define BLOCK_SIZE 8

bool defaultCleanFunction(VoxelOctreeNode* n)
{
  
}

VoxelOctree::VoxelOctree(VoxelData *source, double w, double h, double d, int v_w, int v_h, int v_d) :
  source_(source), w_(w), h_(h), d_(d), v_w_(v_w), v_h_(v_h), v_d_(v_d)
{
  root_ = NULL;
}

VoxelOctree::~VoxelOctree()
{
  if (root_ != NULL) {
    // TODO: Delete children
    delete root_;
  }
}

void VoxelOctree::populateNode(VoxelOctreeNode *node)
{
  // TODO: Read data from data source and fill the node's voxel data,
  // or average values from children if not a leaf.
}

void VoxelOctree::generate()
{
  if (root_ != NULL) {
    // TODO: Delete children
    delete root_;
  }

  root_ = new VoxelOctreeNode();
  root->w = root->h = root->d = BLOCK_SIZE;
  root->x1 = root->y1 = root
}


VoxelOctreeNode *VoxelOctree::getRoot() const
{
  return root_;
}

void VoxelOctree::cleanWith(bool (*function)(VoxelOctreeNode*))
{
  // TODO
}
