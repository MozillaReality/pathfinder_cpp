// pathfinder/src/meshes.cpp
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include "meshes.h"

#include "utils.h"
#include "gl-utils.h"
#include "platform.h"

#include <memory>
#include <assert.h>

using namespace std;

namespace pathfinder {

const __uint32_t RIFF_FOURCC = fourcc("RIFF");
const __uint32_t MESH_PACK_FOURCC = fourcc("PFMP");
const __uint32_t MESH_FOURCC = fourcc("mesh");

const int BBOX_BYTES = (sizeof(float) * 20); // 20 x float32's per index
const int BQII_BYTES = sizeof(__uint32_t);   //  1 x uint32's  per index
const int BQVP_BYTES = (sizeof(float) * 2);  //  2 x float32's per index
const int SSEG_BYTES = (sizeof(float) * 6);  //  6 x float32's per index
const int SNOR_BYTES = (sizeof(float) * 6);  //  6 x float32's per index
const int PATHID_BYTES = sizeof(__uint16_t); //  1 x uint16's  per index

bool
PathfinderMeshPack::load(uint8_t* meshes, size_t meshesLength)
{
  // TODO(kearwood) - Bounds checking
  mMeshes.clear();

  // RIFF encoded data.
  if (readUInt32(meshes, 0) != RIFF_FOURCC) {
    return false;
  }
  if (readUInt32(meshes, 8) != MESH_PACK_FOURCC) {
    return false;
  }

  off_t offset = 12;
  while (offset < meshesLength) {
    __uint32_t fourCC = readUInt32(meshes, offset);
    __uint32_t chunkLength = readUInt32(meshes, offset + 4);
    off_t startOffset = offset + 8;
    off_t endOffset = startOffset + chunkLength;

    if (fourCC == MESH_FOURCC) {
      unique_ptr<PathfinderMesh> mesh = make_unique<PathfinderMesh>();
      mesh->load(meshes + startOffset, endOffset - startOffset);
      mMeshes.push_back(move(mesh));
    }
    offset = endOffset;
  }
  return true;
}

PathfinderMesh::PathfinderMesh()
  : bQuadVertexPositions(nullptr)
  , bQuadVertexPositionsLength(0)
  , bQuadVertexInteriorIndices(nullptr)
  , bQuadVertexInteriorIndicesLength(0)
  , bBoxes(nullptr)
  , bBoxesLength(0)
  , stencilSegments(nullptr)
  , stencilSegmentsLength(0)
  , stencilNormals(nullptr)
  , stencilNormalsLength(0)
{
}

PathfinderMesh::~PathfinderMesh()
{
  clear();
}

bool
PathfinderMesh::load(uint8_t* data, size_t dataLength)
{
  // TODO(kearwood) - Bounds checking and return success/failure bool
  clear();
  off_t offset = 0;
  while (offset < dataLength) {
    __uint32_t fourCC = readUInt32(data, offset);
    __uint32_t chunkLength = readUInt32(data, offset + 4);
    off_t startOffset = offset + 8;
    off_t endOffset = startOffset + chunkLength;

    __uint8_t** dest = nullptr;
    size_t* destLength = nullptr;
    switch (fourCC)
    {
    case fourcc("bbox"): // bBoxes
      dest = &bBoxes;
      destLength = &bBoxesLength;
      break;
    case fourcc("bqii"): // bQuadVertexInteriorIndices
      dest = &bQuadVertexInteriorIndices;
      destLength = &bQuadVertexInteriorIndicesLength;
      break;
    case fourcc("bqvp"): // bQuadVertexPositions
      dest = &bQuadVertexPositions;
      destLength = &bQuadVertexPositionsLength;
      break;
    case fourcc("snor"): // stencilNormals
      dest = &stencilNormals;
      destLength = &stencilNormalsLength;
      break;
    case fourcc("sseg"): // stencilSegments
      dest = &stencilSegments;
      destLength = &stencilSegmentsLength;
      break;
    default:
      // fourcc not recognized
      break;
    }
    if (dest && chunkLength > 0) {
      *dest = (__uint8_t*)malloc(chunkLength);
      memcpy(*dest, data + startOffset, chunkLength);
      *destLength = chunkLength;
    }

    offset = endOffset;
  }
  return true;
}

void
PathfinderMesh::clear()
{
  if (bQuadVertexPositions) {
    delete bQuadVertexPositions;
    bQuadVertexPositions = nullptr;
    bQuadVertexPositionsLength = 0;
  }
  if (bQuadVertexInteriorIndices) {
    delete bQuadVertexInteriorIndices;
    bQuadVertexInteriorIndices = nullptr;
    bQuadVertexInteriorIndicesLength = 0;
  }
  if (bBoxes) {
    delete bBoxes;
    bBoxes = nullptr;
    bBoxesLength = 0;
  }
  if (stencilSegments) {
    delete stencilSegments;
    stencilSegments = nullptr;
    stencilSegmentsLength = 0;
  }
  if (stencilNormals) {
    delete stencilNormals;
    stencilNormals = nullptr;
    stencilNormalsLength = 0;
  }
}

PathfinderPackedMeshes::PathfinderPackedMeshes(const PathfinderMeshPack& meshPack,
                                               vector<int> meshIndices)
  : bQuadVertexPositions(nullptr)
  , bQuadVertexPositionsLength(0)
  , bQuadVertexInteriorIndices(nullptr)
  , bQuadVertexInteriorIndicesLength(0)
  , bBoxes(nullptr)
  , bBoxesLength(0)
  , stencilSegments(nullptr)
  , stencilSegmentsLength(0)
  , stencilNormals(nullptr)
  , stencilNormalsLength(0)
  , bBoxPathIDs(nullptr)
  , bBoxPathIDsLength(0)
  , bQuadVertexPositionPathIDs(nullptr)
  , bQuadVertexPositionPathIDsLength(0)
  , stencilSegmentPathIDs(nullptr)
  , stencilSegmentPathIDsLength(0)
{
  /// NB: Mesh indices are 1-indexed.
  if (meshIndices.size() == 0) {
    for (int i = 0; i < meshPack.mMeshes.size(); i++) {
      meshIndices.push_back(i);
    }
  }

  // ===== First Pass: Populate Ranges and determine buffer sizes =====
  int bbox_total = 0;
  int bqii_total = 0;
  int bqvp_total = 0;
  int sseg_total = 0;
  int snor_total = 0;
  for (int destMeshIndex = 0; destMeshIndex < meshIndices.size(); destMeshIndex++) {
    int srcMeshIndex = meshIndices[destMeshIndex];
    const PathfinderMesh& mesh = *meshPack.mMeshes[srcMeshIndex - 1];

    assert(mesh.bBoxesLength % BBOX_BYTES == 0);
    assert(mesh.bQuadVertexInteriorIndicesLength % BQII_BYTES == 0);
    assert(mesh.bQuadVertexPositionsLength % BQVP_BYTES == 0);
    assert(mesh.stencilSegmentsLength % SSEG_BYTES == 0);
    assert(mesh.stencilNormalsLength % SNOR_BYTES == 0);

    int bbox_count = (int)mesh.bBoxesLength / BBOX_BYTES;
    int bqii_count = (int)mesh.bQuadVertexInteriorIndicesLength / BQII_BYTES;
    int bqvp_count = (int)mesh.bQuadVertexPositionsLength / BQVP_BYTES;
    int sseg_count = (int)mesh.stencilSegmentsLength / SSEG_BYTES;
    int snor_count = (int)mesh.stencilNormalsLength / SNOR_BYTES;

    bBoxPathRanges.push_back(Range(bbox_total, bbox_total + bbox_count));
    bQuadVertexInteriorIndexPathRanges.push_back(Range(bqii_total, bqii_total + bqii_count));
    bQuadVertexPositionPathRanges.push_back(Range(bqvp_total, bqvp_total + bqvp_count));
    stencilSegmentPathRanges.push_back(Range(sseg_total, sseg_total + sseg_count));

    bbox_total += bbox_count;
    bqii_total += bqii_count;
    bqvp_total += bqvp_count;
    sseg_total += sseg_count;
    snor_total += snor_count;
  }

  // ===== Allocate buffers =====
  bBoxes = (__uint8_t*)malloc(bbox_total * BBOX_BYTES);
  bQuadVertexInteriorIndices = (__uint8_t*)malloc(bqii_total * BQII_BYTES);
  bQuadVertexPositions = (__uint8_t*)malloc(bqvp_total * BQVP_BYTES);
  stencilSegments = (__uint8_t*)malloc(sseg_total * SSEG_BYTES);
  stencilNormals = (__uint8_t*)malloc(snor_total * SNOR_BYTES);
  bBoxPathIDs = (__uint8_t*)malloc(bbox_total * PATHID_BYTES);
  bQuadVertexPositionPathIDs = (__uint8_t*)malloc(bqvp_total * PATHID_BYTES);
  stencilSegmentPathIDs = (__uint8_t*)malloc(sseg_total * PATHID_BYTES);

  // ===== Second Pass: Populate buffers =====
  for (int destMeshIndex = 0; destMeshIndex < meshIndices.size(); destMeshIndex++) {
    int srcMeshIndex = meshIndices[destMeshIndex];
    const PathfinderMesh& mesh = *meshPack.mMeshes[srcMeshIndex - 1];

    int bbox_count = (int)mesh.bBoxesLength / BBOX_BYTES;
    int bqii_count = (int)mesh.bQuadVertexInteriorIndicesLength / BQII_BYTES;
    int bqvp_count = (int)mesh.bQuadVertexPositionsLength / BQVP_BYTES;
    int sseg_count = (int)mesh.stencilSegmentsLength / SSEG_BYTES;

    __uint32_t offset = (int)bQuadVertexPositionsLength / BQVP_BYTES;
    for (int i=0; i < bqii_count; i++) {
      __uint32_t index = *((__uint32_t*)mesh.bQuadVertexInteriorIndices + i);
      *((__uint32_t*)(bQuadVertexInteriorIndices + bQuadVertexInteriorIndicesLength)) = index + offset;
      bQuadVertexInteriorIndicesLength += BQII_BYTES;
    }

    memcpy(bQuadVertexPositions + bQuadVertexPositionsLength, mesh.bQuadVertexPositions, mesh.bQuadVertexPositionsLength);
    memcpy(bBoxes + bBoxesLength, mesh.bBoxes, mesh.bBoxesLength);
    memcpy(stencilSegments + stencilSegmentsLength, mesh.stencilSegments, mesh.stencilSegmentsLength);
    memcpy(stencilNormals + stencilNormalsLength, mesh.stencilNormals, mesh.stencilNormalsLength);

    bQuadVertexPositionsLength += mesh.bQuadVertexPositionsLength;
    bBoxesLength += mesh.bBoxesLength;
    stencilSegmentsLength += mesh.stencilSegmentsLength;
    stencilNormalsLength += mesh.stencilNormalsLength;

    for (int i = 0; i < bbox_count; i++) {
      *((__uint16_t*)(bBoxPathIDs + bBoxPathIDsLength)) = destMeshIndex + 1;
      bBoxPathIDsLength += PATHID_BYTES;
    }
    for (int i = 0; i < bqvp_count; i++) {
      *((__uint16_t*)(bQuadVertexPositionPathIDs + bQuadVertexPositionPathIDsLength)) = destMeshIndex + 1;
      bQuadVertexPositionPathIDsLength += PATHID_BYTES;
    }
    for (int i = 0; i < sseg_count; i++) {
      *((__uint16_t*)(stencilSegmentPathIDs + stencilSegmentPathIDsLength)) = destMeshIndex + 1;
      stencilSegmentPathIDsLength += PATHID_BYTES;
    }
  }
}

int
PathfinderPackedMeshes::stencilSegmentsCount() const {
  return (int)stencilSegmentsLength / SSEG_BYTES;
}

PathfinderPackedMeshes::~PathfinderPackedMeshes()
{
  if (bQuadVertexPositions) {
    free(bQuadVertexPositions);
    bQuadVertexPositions = nullptr;
  }
  if (bBoxes) {
    free(bBoxes);
    bBoxes = nullptr;
  }
  if (stencilSegments) {
    free(stencilSegments);
    stencilSegments = nullptr;
  }
  if (stencilNormals) {
    free(stencilNormals);
    stencilNormals = nullptr;
  }
  if (bBoxPathIDs) {
    free(bBoxPathIDs);
    bBoxPathIDs = nullptr;
  }
  if (bQuadVertexPositionPathIDs) {
    free(bQuadVertexPositionPathIDs);
    bQuadVertexPositionPathIDs = nullptr;
  }
  if (stencilSegmentPathIDs) {
    free(stencilSegmentPathIDs);
    stencilSegmentPathIDs = nullptr;
  }

  bQuadVertexPositionsLength = 0;
  bBoxesLength = 0;
  stencilSegmentsLength = 0;
  stencilNormalsLength = 0;
  bBoxPathIDsLength = 0;
  bQuadVertexPositionPathIDsLength = 0;
  stencilSegmentPathIDsLength = 0;
}

PathfinderPackedMeshBuffers::PathfinderPackedMeshBuffers(const PathfinderPackedMeshes& packedMeshes)
 : bBoxes(0)
 , bQuadVertexInteriorIndices(0)
 , bQuadVertexPositions(0)
 , stencilSegments(0)
 , stencilNormals(0)
 , bBoxPathIDs(0)
 , bQuadVertexPositionPathIDs(0)
 , stencilSegmentPathIDs(0)
{
  // TODO(kearwood) - This can be less repetitive with some macros.
  // also can use one glCreateBuffers call of all buffers
  GLDEBUG(glCreateBuffers(1, &bBoxPathIDs));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, bBoxPathIDs));
  GLDEBUG(glBufferData(GL_ARRAY_BUFFER, packedMeshes.bBoxPathIDsLength, packedMeshes.bBoxPathIDs, GL_STATIC_DRAW));

  GLDEBUG(glCreateBuffers(1, &bBoxes));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, bBoxes));
  GLDEBUG(glBufferData(GL_ARRAY_BUFFER, packedMeshes.bBoxesLength, packedMeshes.bBoxes, GL_STATIC_DRAW));

  GLDEBUG(glCreateBuffers(1, &bQuadVertexInteriorIndices));
  GLDEBUG(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bQuadVertexInteriorIndices));
  GLDEBUG(glBufferData(GL_ELEMENT_ARRAY_BUFFER, packedMeshes.bQuadVertexInteriorIndicesLength, packedMeshes.bQuadVertexInteriorIndices, GL_STATIC_DRAW));

  GLDEBUG(glCreateBuffers(1, &bQuadVertexPositionPathIDs));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, bQuadVertexPositionPathIDs));
  GLDEBUG(glBufferData(GL_ARRAY_BUFFER, packedMeshes.bQuadVertexPositionPathIDsLength, packedMeshes.bQuadVertexPositionPathIDs, GL_STATIC_DRAW));

  GLDEBUG(glCreateBuffers(1, &bQuadVertexPositions));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, bQuadVertexPositions));
  GLDEBUG(glBufferData(GL_ARRAY_BUFFER, packedMeshes.bQuadVertexPositionsLength, packedMeshes.bQuadVertexPositions, GL_STATIC_DRAW));

  GLDEBUG(glCreateBuffers(1, &stencilNormals));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, stencilNormals));
  GLDEBUG(glBufferData(GL_ARRAY_BUFFER, packedMeshes.stencilNormalsLength, packedMeshes.stencilNormals, GL_STATIC_DRAW));

  GLDEBUG(glCreateBuffers(1, &stencilSegmentPathIDs));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, stencilSegmentPathIDs));
  GLDEBUG(glBufferData(GL_ARRAY_BUFFER, packedMeshes.stencilSegmentPathIDsLength, packedMeshes.stencilSegmentPathIDs, GL_STATIC_DRAW));

  GLDEBUG(glCreateBuffers(1, &stencilSegments));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, stencilSegments));
  GLDEBUG(glBufferData(GL_ARRAY_BUFFER, packedMeshes.stencilSegmentsLength, packedMeshes.stencilSegments, GL_STATIC_DRAW));

  bBoxPathRanges = packedMeshes.bBoxPathRanges;
  bQuadVertexInteriorIndexPathRanges = packedMeshes.bQuadVertexInteriorIndexPathRanges;
  bQuadVertexPositionPathRanges = packedMeshes.bQuadVertexPositionPathRanges;
  stencilSegmentPathRanges = packedMeshes.stencilSegmentPathRanges;
}

PathfinderPackedMeshBuffers::~PathfinderPackedMeshBuffers()
{
  if (bBoxes) {
    GLDEBUG(glDeleteBuffers(1, &bBoxes));
    bBoxes = 0;
  }
  if (bQuadVertexInteriorIndices) {
    GLDEBUG(glDeleteBuffers(1, &bQuadVertexInteriorIndices));
    bQuadVertexInteriorIndices = 0;
  }
  if (bQuadVertexPositions) {
    GLDEBUG(glDeleteBuffers(1, &bQuadVertexPositions));
    bQuadVertexPositions = 0;
  }
  if (stencilSegments) {
    GLDEBUG(glDeleteBuffers(1, &stencilSegments));
    stencilSegments = 0;
  }
  if (stencilNormals) {
    GLDEBUG(glDeleteBuffers(1, &stencilNormals));
    stencilNormals = 0;
  }
  if (bBoxPathIDs) {
    GLDEBUG(glDeleteBuffers(1, &bBoxPathIDs));
    bBoxPathIDs = 0;
  }
  if (bQuadVertexPositionPathIDs) {
    GLDEBUG(glDeleteBuffers(1, &bQuadVertexPositionPathIDs));
    bQuadVertexPositionPathIDs = 0;
  }
  if (stencilSegmentPathIDs) {
    GLDEBUG(glDeleteBuffers(1, &stencilSegmentPathIDs));
    stencilSegmentPathIDs = 0;
  }
}

__uint32_t
readUInt32(uint8_t* buffer, off_t offset)
{
  return *((__uint32_t*)(buffer + offset));
}

} // namespace pathfinder
