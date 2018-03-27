// pathfinder/src/meshes.h
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#ifndef PATHFINDER_MESHES_H
#define PATHFINDER_MESHES_H

#include "utils.h"

#include <vector>
#include <memory>

namespace pathfinder {

class PathRanges
{
public:
  std::vector<Range> bBoxPathRanges;
  std::vector<Range> bQuadVertexInteriorIndexPathRanges;
  std::vector<Range> bQuadVertexPositionPathRanges;
  std::vector<Range> stencilSegmentPathRanges;
};

class PathfinderMesh
{
public:
  PathfinderMesh();
  ~PathfinderMesh();
  PathfinderMesh(const PathfinderMesh& other) = delete;

  bool load(uint8_t* data, size_t dataLength);

  // bqvp data
  __uint8_t* bQuadVertexPositions;
  size_t bQuadVertexPositionsLength;

  // bqii data
  __uint8_t* bQuadVertexInteriorIndices;
  size_t bQuadVertexInteriorIndicesLength;

  // bbox data
  __uint8_t* bBoxes;
  size_t bBoxesLength;

  // sseg data
  __uint8_t* stencilSegments;
  size_t stencilSegmentsLength;

  // snor data
  __uint8_t* stencilNormals;
  size_t stencilNormalsLength;
private:
  void clear();
};

class PathfinderMeshPack
{
public:
  // Explicity delete copy constructor
  PathfinderMeshPack(const PathfinderMeshPack& other) = delete;

  bool load(uint8_t* meshes, size_t meshesLength);

  std::vector<std::unique_ptr<PathfinderMesh>> mMeshes;
};

class PathfinderPackedMeshes : public PathRanges
{
public:
  PathfinderPackedMeshes(const PathfinderMeshPack& meshPack, std::vector<int> meshIndices);
  ~PathfinderPackedMeshes();
  // Explicity delete copy constructor
  PathfinderPackedMeshes(const PathfinderPackedMeshes& other) = delete;

  // bqvp data
  __uint8_t* bQuadVertexPositions;
  size_t bQuadVertexPositionsLength;

  // bqii data
  __uint8_t* bQuadVertexInteriorIndices;
  size_t bQuadVertexInteriorIndicesLength;

  // bbox data
  __uint8_t* bBoxes;
  size_t bBoxesLength;

  // sseg data
  __uint8_t* stencilSegments;
  size_t stencilSegmentsLength;

  // snor data
  __uint8_t* stencilNormals;
  size_t stencilNormalsLength;

  __uint8_t* bBoxPathIDs;
  size_t bBoxPathIDsLength;

  __uint8_t* bQuadVertexPositionPathIDs;
  size_t bQuadVertexPositionPathIDsLength;

  __uint8_t* stencilSegmentPathIDs;
  size_t stencilSegmentPathIDsLength;

  int stencilSegmentsCount() const;
};

class PathfinderPackedMeshBuffers : public PathRanges
{
public:
  PathfinderPackedMeshBuffers(const PathfinderPackedMeshes& packedMeshes);

  GLuint bBoxes;
  GLuint bQuadVertexInteriorIndices;
  GLuint bQuadVertexPositions;
  GLuint stencilSegments;
  GLuint stencilNormals;

  GLuint bBoxPathIDs;
  GLuint bQuadVertexPositionPathIDs;
  GLuint stencilSegmentPathIDs;
};

__uint32_t readUInt32(uint8_t* buffer, off_t offset);

} // namespace pathfinder

#endif // PATHFINDER_MESHES_H
