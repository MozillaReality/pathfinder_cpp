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


PathfinderPackedMeshes::PathfinderPackedMeshes(const PathfinderMeshPack& meshPack,
                                               vector<int> meshIndices)
{
  /// NB: Mesh indices are 1-indexed.
  if (meshIndices.size() == 0) {
    meshIndices = meshPack.mMeshes.map((value, index) => index + 1);
  }

  PackedMeshBuilder<number[]> meshData;
  /*:  = {
      bBoxPathIDs: [],
      bBoxes: [],
      bQuadVertexInteriorIndices: [],
      bQuadVertexPositionPathIDs: [],
      bQuadVertexPositions: [],
      stencilNormals: [],
      stencilSegmentPathIDs: [],
      stencilSegments: [],
  };
  */

  PathRanges pathRanges;

  for (int destMeshIndex = 0; destMeshIndex < meshIndices.length; destMeshIndex++) {
    int srcMeshIndex = meshIndices[destMeshIndex];
    const PathfinderMesh& mesh = meshPack.mMeshes[srcMeshIndex - 1];

    // ----- Set Range start indices -----
    int startIndex = meshData.bBoxes.size() / (20 * 4); // 20 x float32's
    pathRanges.bBoxPathRanges.push_back(Range(startIndex, startIndex));
    startIndex = meshData.bQuadVertexInteriorIndices.size() / (1 * 4); // 1 x uint32's
    pathRanges.bQuadVertexInteriorIndexPathRanges.push_back(Range(startIndex, startIndex));
    startIndex = meshData.bQuadVertexPositions.size() / (2 * 4); // 2 x float32's
    pathRanges.bQuadVertexPositionPathRanges.push_back(Range(startIndex, startIndex));
    startIndex = meshData.stencilSegments.size() / (6 * 4); // 6 x float32's
    pathRanges.stencilSegmentPathRanges.push_back(Range(startIndex, startIndex));

    __uint32_t offset = meshData.bQuadVertexPositions.size() / (2 * 4); // 2 x float32's
    for (int i=0; i < mesh.bQuadVertexInteriorIndicesLength / 4; i++) {
      __uint32_t index = ((__uint32_t*)mesh.bQuadVertexInteriorIndices) + i;
      meshData.bQuadVertexPositions.push_back(index + offset);
    }

    meshData.bBoxPathIDs.push_back(mesh.bBoxPathIDs);
    meshData.bBoxes.push_back(mesh.bBoxes);
    meshData.bQuadVertexPositionPathIDs.push_back(mesh.bQuadVertexPositionPathIDs);
    meshData.bQuadVertexPositions.push_back(mesh.bQuadVertexPositions);
    meshData.stencilNormals.push_back(mesh.stencilNormals);
    meshData.stencilSegmentPathIDs.push_back(mesh.stencilSegmentPathIDs);
    meshData.stencilSegments.push_back(mesh.stencilSegments);

    size_t length = meshData.bBoxes.size() / 20; // 20 x float32's
    while (meshData.bBoxes.length < length) {
      meshData.bBoxes.push_back(destMeshIndex + 1);
    }
    length = meshData.bQuadVertexPositions.size() / 2; // 2 x float32's
    while (meshData.bQuadVertexPositions.length < length) {
      meshData.bQuadVertexPositions.push_back(destMeshIndex + 1);
    }
    length = meshData.stencilSegments.size() / 6; // 6 x float32's
    while (meshData.stencilSegments.length < length) {
      meshData.stencilSegments.push_back(destMeshIndex + 1);
    }

    // ----- Set Range end indices -----
    int endIndex = meshData.bBoxes.size() / (20 * 4); // 20 x float32's
    pathRanges.bBoxPathRanges.back().end = endIndex;
    endIndex = meshData.bQuadVertexInteriorIndices.size() / (1 * 4); // 1 x uint32's
    pathRanges.bQuadVertexInteriorIndexPathRanges.back().end = endIndex;
    endIndex = meshData.bQuadVertexPositions.size() / (2 * 4); // 2 x float32's
    pathRanges.bQuadVertexPositionPathRanges.back().end = endIndex;
    endIndex = meshData.stencilSegments.size() / (6 * 4); // 6 x float32's
    pathRanges.stencilSegmentPathRanges.back().end = endIndex;
  }


/*
    bBoxPathIDs - Uint16 - Uint16Array
    bBoxes - Float32 - Uint32Array
    bQuadVertexInteriorIndices - Uint32 - Uint32Array
    bQuadVertexPositionPathIDs - Uint16 - Uint16Array
    bQuadVertexPositions - Float32 - Uint32Array
    stencilNormals - Float32 - Uint32Array
    stencilSegmentPathIDs - Uint16 - Uint16Array
    stencilSegments - Float32 - Uint32Array
*/

  for (const bufferType of Object.keys(BUFFER_TYPES) as PackedMeshBufferType[]) {
      const arrayCtor = PRIMITIVE_TYPE_ARRAY_CONSTRUCTORS[MESH_TYPES[bufferType].type];
      this[bufferType] = (new arrayCtor(meshData[bufferType])) as any;
  }
  _.assign(this, pathRanges);
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
  GLDEBUG(glBufferData(GL_ARRAY_BUFFER, packedMeshes.bBoxPathIDs, packedMeshes.bBoxPathIDsLength, GL_STATIC_DRAW));

  GLDEBUG(glCreateBuffers(1, &bBoxes));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, bBoxes));
  GLDEBUG(glBufferData(GL_ARRAY_BUFFER, packedMeshes.bBoxes, packedMeshes.bBoxesLength, GL_STATIC_DRAW));

  GLDEBUG(glCreateBuffers(1, &bQuadVertexInteriorIndices));
  GLDEBUG(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bQuadVertexInteriorIndices));
  GLDEBUG(glBufferData(GL_ELEMENT_ARRAY_BUFFER, packedMeshes.bQuadVertexInteriorIndices, packedMeshes.bQuadVertexInteriorIndicesLength, GL_STATIC_DRAW));

  GLDEBUG(glCreateBuffers(1, &bQuadVertexPositionPathIDs));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, bQuadVertexPositionPathIDs));
  GLDEBUG(glBufferData(GL_ARRAY_BUFFER, packedMeshes.bQuadVertexPositionPathIDs, packedMeshes.bQuadVertexPositionPathIDsLength, GL_STATIC_DRAW));

  GLDEBUG(glCreateBuffers(1, &bQuadVertexPositions));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, bQuadVertexPositions));
  GLDEBUG(glBufferData(GL_ARRAY_BUFFER, packedMeshes.bQuadVertexPositions, packedMeshes.bQuadVertexPositionsLength, GL_STATIC_DRAW));

  GLDEBUG(glCreateBuffers(1, &stencilNormals));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, stencilNormals));
  GLDEBUG(glBufferData(GL_ARRAY_BUFFER, packedMeshes.stencilNormals, packedMeshes.stencilNormalsLength, GL_STATIC_DRAW));

  GLDEBUG(glCreateBuffers(1, &stencilSegmentPathIDs));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, stencilSegmentPathIDs));
  GLDEBUG(glBufferData(GL_ARRAY_BUFFER, packedMeshes.stencilSegmentPathIDs, packedMeshes.stencilSegmentPathIDsLength, GL_STATIC_DRAW));

  GLDEBUG(glCreateBuffers(1, &stencilSegments));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, stencilSegments));
  GLDEBUG(glBufferData(GL_ARRAY_BUFFER, packedMeshes.stencilSegments, packedMeshes.stencilSegmentsLength, GL_STATIC_DRAW));

  bBoxPathRanges = packedMeshes.bBoxPathRanges;
  bQuadVertexInteriorIndexPathRanges = packedMeshes.bQuadVertexInteriorIndexPathRanges;
  bQuadVertexPositionPathRanges = packedMeshes.bQuadVertexPositionPathRanges;
  stencilSegmentPathRanges = packedMeshes.stencilSegmentPathRanges;
}

__uint32_t
readUInt32(uint8_t* buffer, off_t offset)
{
  return *((__uint32_t*)(buffer + offset));
}

} // namespace pathfinder
