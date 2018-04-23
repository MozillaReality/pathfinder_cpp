// pathfinder/src/renderer.h
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#ifndef PATHFINDER_RENDERER_H
#define PATHFINDER_RENDERER_H

#include "platform.h"
#include <hydra.h>
#include <vector>
#include <memory>

#include "utils.h"
#include "aa-strategy.h"
#include "shader-loader.h"

namespace pathfinder {

const int TIME_INTERVAL_DELAY = 32;

const int B_LOOP_BLINN_DATA_SIZE = 4;
const int B_LOOP_BLINN_DATA_TEX_COORD_OFFSET = 0;
const int B_LOOP_BLINN_DATA_SIGN_OFFSET = 2;

template <class T>
struct PathTransformBuffers {
  PathTransformBuffers(std::shared_ptr<T> aSt, std::shared_ptr<T> aExt)
   : st(aSt)
   , ext(aExt)
  { }
  std::shared_ptr<T> st;
  std::shared_ptr<T> ext;
};

class RenderContext;
class PathfinderBufferTexture;
class PathfinderPackedMeshBuffers;
class PathfinderPackedMeshes;

class Renderer
{
public:
  Renderer(std::shared_ptr<RenderContext> renderContext);
  virtual ~Renderer();
  Renderer(const Renderer&) = delete;
  Renderer& operator=(const Renderer&) = delete;
  virtual bool init(AntialiasingStrategyName aaType,
                    int aaLevel,
                    AAOptions aaOptions);
  std::shared_ptr<RenderContext> getRenderContext() const {
    return mRenderContext;
  }
  virtual kraken::Vector2 getTotalEmboldenAmount() const {
    return kraken::Vector2::Zero();
  }
  virtual kraken::Vector4 getBGColor() const {
    return kraken::Vector4::Create(0.0f, 0.0f, 0.0f, 1.0f);
  }
  virtual kraken::Vector4 getFGColor() const {
    return kraken::Vector4::One();
  }
  bool getMeshesAttached() const {
    return mMeshBuffers.size() > 0 && mMeshes.size() > 0;
  }
  const std::vector<std::shared_ptr<PathfinderPackedMeshBuffers>>& getMeshBuffers() const{
    return mMeshBuffers;
  }
  std::vector<std::shared_ptr<PathfinderPackedMeshes>>& getMeshes() {
    return mMeshes;
  }
  virtual bool getIsMulticolor() const = 0;
  virtual bool getNeedsStencil() const = 0;
  virtual GLuint getAtlasFramebuffer() const = 0;
  virtual kraken::Vector2i getAtlasAllocatedSize() const = 0;
  virtual kraken::Vector2i getAtlasUsedSize() const = 0;

  void attachMeshes(std::vector<std::shared_ptr<PathfinderPackedMeshes>>& meshes);

  virtual std::shared_ptr<std::vector<float>> pathBoundingRects(int objectIndex) = 0;
  virtual void setHintsUniform(PathfinderShaderProgram& aProgram) = 0;
  virtual void draw(kraken::Matrix4 aTransform) = 0;

  void setFramebufferSizeUniform(PathfinderShaderProgram& aProgram);
  void setTransformAndTexScaleUniformsForDest(PathfinderShaderProgram& aProgram, TileInfo* tileInfo);
  void setTransformSTAndTexScaleUniformsForDest(PathfinderShaderProgram& aProgram);
  void setTransformUniform(PathfinderShaderProgram& aProgram, int pass, int objectIndex);
  void setTransformSTUniform(PathfinderShaderProgram& aProgram, int objectIndex);
  void setTransformAffineUniforms(PathfinderShaderProgram& aProgram, int objectIndex);
  void uploadPathColors(int objectCount);
  void uploadPathTransforms(int objectCount);
  void setPathColorsUniform(int objectIndex, PathfinderShaderProgram& aProgram, GLuint textureUnit);
  void setEmboldenAmountUniform(int objectIndex, PathfinderShaderProgram& aProgram);
  int meshIndexForObject(int objectIndex);
  Range pathRangeForObject(int objectIndex);
  std::vector<std::shared_ptr<PathTransformBuffers<PathfinderBufferTexture>>>& getPathTransformBufferTextures() { return mPathTransformBufferTextures; }
  void bindGammaLUT(kraken::Vector3 bgColor, GLuint textureUnit, PathfinderShaderProgram& aProgram);
  void bindAreaLUT(GLuint textureUnit, PathfinderShaderProgram& aProgram);

protected:
  std::shared_ptr<RenderContext> mRenderContext;
  std::shared_ptr<AntialiasingStrategy> mAntialiasingStrategy;
  std::vector<std::shared_ptr<PathfinderBufferTexture>> mPathColorsBufferTextures;
  GammaCorrectionMode mGammaCorrectionMode;
  bool getPathIDsAreInstanced() {
    return false;
  }
  void renderAtlas();
  virtual int getObjectCount() const = 0;
  virtual kraken::Vector2 getUsedSizeFactor() const = 0;
  virtual kraken::Matrix4 getWorldTransform() const = 0;
  kraken::Vector4 clearColorForObject(int objectIndex) {
    return kraken::Vector4::Zero();
  }
  virtual std::shared_ptr<AntialiasingStrategy> createAAStrategy(AntialiasingStrategyName aaType,
                                        int aaLevel,
                                        SubpixelAAType subpixelAA,
                                        StemDarkeningMode stemDarkening) = 0;
  virtual std::vector<__uint8_t> pathColorsForObject(int objectIndex) = 0;
  virtual std::shared_ptr<PathTransformBuffers<std::vector<float>>> pathTransformsForObject(int objectIndex) = 0;

  virtual ProgramID getDirectCurveProgramName() = 0;
  virtual ProgramID getDirectInteriorProgramName(DirectRenderingMode renderingMode) = 0;

  void clearDestFramebuffer();
  void clearForDirectRendering(int objectIndex);
  kraken::Matrix4 getModelviewTransform(int pathIndex);

  /// If non-instanced, returns instance 0. An empty range skips rendering the object entirely.
  Range instanceRangeForObject(int objectIndex);

  std::shared_ptr<PathTransformBuffers<std::vector<float>>> createPathTransformBuffers(int pathCount);

  void setAntialiasingOptions(AntialiasingStrategyName aaType,
                              int aaLevel,
                              AAOptions aaOptions);

  std::vector<std::shared_ptr<PathfinderPackedMeshes>> mMeshes;
private:

  void directlyRenderObject(int pass, int objectIndex);
  void initImplicitCoverCurveVAO(int objectIndex, Range instanceRange);
  void initImplicitCoverInteriorVAO(int objectIndex, Range instanceRange, DirectRenderingMode renderingMode);
  kraken::Matrix4 computeTransform(int pass, int objectIndex);


  std::vector<std::shared_ptr<PathTransformBuffers<PathfinderBufferTexture>>> mPathTransformBufferTextures;
  std::vector<std::shared_ptr<PathfinderPackedMeshBuffers>> mMeshBuffers;

  GLuint mImplicitCoverInteriorVAO;
  GLuint mImplicitCoverCurveVAO;
};

Range getMeshIndexRange(const std::vector<Range>& indexRanges, Range pathRange);

} // namespace pathfinder

#endif // PATHFINDER_RENDERER_H

