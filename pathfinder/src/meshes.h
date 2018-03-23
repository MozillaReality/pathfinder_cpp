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
  PathfinderMeshPack();
  ~PathfinderMeshPack();

  // Explicity delete copy constructor
  PathfinderMeshPack(const PathfinderMeshPack& other) = delete;

  bool load(uint8_t* meshes, size_t meshesLength);

  std::vector<std::unique_ptr<PathfinderMesh>> mMeshes;
};

class PathfinderPackedMeshes : public PathRanges
{
public:
  PathfinderPackedMeshes(const PathfinderMeshPack& meshPack, std::vector<int> meshIndices);
  // Explicity delete copy constructor
  PathfinderPackedMeshes(const PathfinderPackedMeshes& other) = delete;

  std::vector<float> bBoxes;
  std::vector<__uint32_t> bQuadVertexInteriorIndices;
  std::vector<float> bQuadVertexPositions;
  std::vector<float> stencilSegments;
  std::vector<float> stencilNormals;

  std::vector<__uint16_t> bBoxPathIDs;
  std::vector<__uint16_t> bQuadVertexPositionPathIDs;
  std::vector<__uint16_t> stencilSegmentPathIDs;
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
