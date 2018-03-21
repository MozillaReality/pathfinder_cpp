#include "meshes.h"

#include "utils.h"

using namespace std;

namespace pathfinder {

PathfinderMeshPack::PathfinderMeshPack()
{

}

PathfinderMeshPack::~PathfinderMeshPack()
{

}

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
      mMeshes.push_back(make_shared<PathfinderMesh>(meshes + startOffset, endOffset - startOffset));
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

    GLvoid* dest = nullptr;
    size_t* destLength = nullptr;
    switch (fourCC)
    {
    case fourcc("bbox"): // bBoxes
      dest = bBoxes
      destLength = &bBoxesLength;
      break;
    case fourcc("bqii"): // bQuadVertexInteriorIndices
      dest = bQuadVertexInteriorIndices;
      destLength = &bQuadVertexInteriorIndicesLength;
      break;
    case fourcc("bqvp"): // bQuadVertexPositions
      dest = bQuadVertexPositions;
      destLength = &bQuadVertexPositionsLength;
      break;
    case fourcc("snor"): // stencilNormals
      dest = stencilNormals;
      destLength = &stencilNormalsLength;
      break;
    case fourcc("sseg"): // stencilSegments
      dest = stencilSegments;
      destLength = &stencilSegmentsLength;
      break;
    default:
      // fourcc not recognized
      break;
    }
    if (dest && destLength) {
      dest = malloc(destLength);
      memcpy(dest, data + startOffset, chunkLength);
      destLength = chunkLength;
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

__uint32_t
readUInt32(uint8_t* buffer, off_t offset)
{
  return *((__uint32_t*)(buffer + offset));
}

} // namespace pathfinder
