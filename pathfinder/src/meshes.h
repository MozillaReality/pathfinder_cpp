#ifndef PATHFINDER_MESHES_H
#define PATHFINDER_MESHES_H

namespace pathfinder {


struct VertexCopyResult
{
  int originalStartIndex;
  int originalEndIndex;
  int expandedStartIndex;
  int expandedEndIndex;
};

typedef enum {
  bt_ARRAY_BUFFER,
  bt_ELEMENT_ARRAY_BUFFER
} BufferType;

class ArrayLike
{
public:
  virtual int length() = 0;
};

struct VertexCopyResult
{
  int originalStartIndex;
  int originalEndIndex;
  int expandedStartIndex;
  int expandedEndIndex;
};

typedef enum
{
  pt_Uint16,
  pt_Uint32,
  pt_Float32
} PrimitiveType;

const int B_QUAD_SIZE = 4 * 8;
const int B_QUAD_UPPER_LEFT_VERTEX_OFFSET = 4 * 0;
const int B_QUAD_UPPER_RIGHT_VERTEX_OFFSET = 4 * 1;
const int B_QUAD_UPPER_CONTROL_POINT_VERTEX_OFFSET = 4 * 2;
const int B_QUAD_LOWER_LEFT_VERTEX_OFFSET = 4 * 4;
const int B_QUAD_LOWER_RIGHT_VERTEX_OFFSET = 4 * 5;
const int B_QUAD_LOWER_CONTROL_POINT_VERTEX_OFFSET = 4 * 6;
const int B_QUAD_UPPER_INDICES_OFFSET = B_QUAD_UPPER_LEFT_VERTEX_OFFSET;
const int B_QUAD_LOWER_INDICES_OFFSET = B_QUAD_LOWER_LEFT_VERTEX_OFFSET;

const int B_QUAD_FIELD_COUNT = B_QUAD_SIZE / UINT32_SIZE;

// FIXME(pcwalton): This duplicates information below in `MESH_TYPES`.
const int INDEX_SIZE = 4;
const int B_QUAD_VERTEX_POSITION_SIZE = 12 * 4;
const int B_VERTEX_POSITION_SIZE = 4 * 2;

const __uint32_t RIFF_FOURCC = fourcc('RIFF');
const __uint32_t MESH_PACK_FOURCC = fourcc('PFMP');
const __uint32_t MESH_FOURCC = fourcc('mesh');

struct MeshBufferTypeDescriptor
{
  PrimitiveType type;
  int size;
};

template <class T>
class MeshBuilder
{
public:
  T bQuadVertexPositions;
  T bQuadVertexInteriorIndices;
  T bBoxes;
  T stencilSegments;
  T stencilNormals;
};

template <class T>
class PackedMeshBuilder : public MeshBuilder<T>
{
public:
  T bBoxPathIDs;
  T bQuadVertexPositionPathIDs;
  T stencilSegmentPathIDs;
};

class PathRanges
{
public:
  virtual std::vector<Range> bBoxPathRanges() = 0;
  virtual std::vector<Range> bQuadVertexInteriorIndexPathRanges() = 0;
  virtual std::vector<Range> bQuadVertexPositionPathRanges() = 0;
  virtual std::vector<Range> stencilSegmentPathRanges() = 0;
};

class MeshDataCounts
{
public:
  virtual int bQuadVertexPositionCount() = 0;
  virtual int bQuadVertexInteriorIndexCount() = 0;
  virtual int bBoxCount() = 0;
  virtual int stencilSegmentCount() = 0;
};

class IndexTypeDescriptor
{
public:
    MeshBufferType bufferType;
};

class PathfinderMesh
{
public:
  PathfinderMesh();
  ~PathfinderMesh();
  bool load(uint8_t* data, size_t dataLength);

  GLvoid* bQuadVertexPositions;
  size_t bQuadVertexPositionsLength;
  GLvoid* bQuadVertexInteriorIndices;
  size_t bQuadVertexInteriorIndicesLength;
  GLvoid* bBoxes;
  size_t bBoxesLength;
  GLvoid* stencilSegments;
  size_t stencilSegmentsLength;
  GLvoid* stencilNormals;
  size_T stencilNormalsLength;
private:
  clear();
};

class PathfinderMeshPack
{
public:
  PathfinderMeshPack();
  ~PathfinderMeshPack();

  bool load(uint8_t* meshes, size_t meshesLength);

  std::vector<PathfinderMesh> mMeshes;
};
  

__uint32_t readUInt32(uint8_t* buffer, off_t offset);

} // namespace pathfinder

#endif // PATHFINDER_MESHES_H
