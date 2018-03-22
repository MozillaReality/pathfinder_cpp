#include "renderer.h"

#include "resources.h"

#include "render-context.h"
#include "aa-strategy.h"
#include "gl-utils.h"
#include "buffer-texture.h"
#include "meshes.h"

#include <assert.h>

using namespace std;
using namespace kraken;

namespace pathfinder {

Renderer::Renderer(shared_ptr<RenderContext> renderContext)
 : mRenderContext(renderContext)
 , mGammaCorrectionMode(gcm_on)
 , mImplicitCoverInteriorVAO(0)
 , mImplicitCoverCurveVAO(0)
 , mGammaLUTTexture(0)
 , mInstancedPathIDVBO(0)
 , mVertexIDVBO(0)
{
  if (getPathIDsAreInstanced()) {
    initInstancedPathIDVBO();
  }
  initVertexIDVBO();
  initGammaLUTTexture();

  mAntialiasingStrategy = make_shared<NoAAStrategy>(0, spaa_none);
  mAntialiasingStrategy->init(*this);
  mAntialiasingStrategy->setFramebufferSize(*this);
}

Renderer::~Renderer()
{
}

void
Renderer::attachMeshes(vector<shared_ptr<PathfinderPackedMeshes>>& meshes)
{
  assert(mAntialiasingStrategy);
  mMeshes = meshes;
  mMeshBuffers.clear();
  for (shared_ptr<PathfinderPackedMeshes>& m: meshes) {
    mMeshBuffers.push_back(move(make_shared<PathfinderPackedMeshBuffers>(m)));
  }
  mAntialiasingStrategy->attachMeshes(*this);
}

void
Renderer::redraw()
{
  if (mMeshBuffers.size() == 0) {
    return;
  }

  clearDestFramebuffer();

  assert(mAntialiasingStrategy);
  mAntialiasingStrategy->prepareForRendering(*this);

  // Draw "scenery" (used in the 3D view).
  drawSceneryIfNecessary();

  int passCount = mAntialiasingStrategy->getPassCount();
  for (int pass = 0; pass < passCount; pass++) {
    if (mAntialiasingStrategy->getDirectRenderingMode() != drm_none) {
      mAntialiasingStrategy->prepareForDirectRendering(*this);
    }

    int objectCount = getObjectCount();
    for (int objectIndex = 0; objectIndex < objectCount; objectIndex++) {
      if (mAntialiasingStrategy->getDirectRenderingMode() != drm_none) {
        // Prepare for direct rendering.
        mAntialiasingStrategy->prepareToRenderObject(*this, objectIndex);

        // Clear.
        clearForDirectRendering(objectIndex);

        // Perform direct rendering (Loop-Blinn).
        directlyRenderObject(pass, objectIndex);
      }

      // Antialias.
      mAntialiasingStrategy->antialiasObject(*this, objectIndex);

      // Perform post-antialiasing tasks.
      mAntialiasingStrategy->finishAntialiasingObject(*this, objectIndex);

      mAntialiasingStrategy->resolveAAForObject(*this, objectIndex);
    }

    mAntialiasingStrategy->resolve(pass, *this);
  }

  // Draw the glyphs with the resolved atlas to the default framebuffer.
  compositeIfNecessary();
}

void
Renderer::setAntialiasingOptions(AntialiasingStrategyName aaType,
                                 int aaLevel,
                                 AAOptions aaOptions)
{
  mGammaCorrectionMode = aaOptions.gammaCorrection;

  mAntialiasingStrategy = createAAStrategy(aaType,
                                           aaLevel,
                                           aaOptions.subpixelAA,
                                           aaOptions.stemDarkening);

  mAntialiasingStrategy->init(*this);
  if (mMeshes.size() != 0) {
    mAntialiasingStrategy->attachMeshes(*this);
  }
  mAntialiasingStrategy->setFramebufferSize(*this);
  mRenderContext->setDirty();
}

void
Renderer::canvasResized()
{
  if (mAntialiasingStrategy) {
    mAntialiasingStrategy->init(*this);
  }
}

void
Renderer::setFramebufferSizeUniform(const UniformMap& uniforms)
{
  Vector2 destAllocatedSize = getDestAllocatedSize();
  glUniform2i(uniforms.uFramebufferSize,
              destAllocatedSize[0],
              destAllocatedSize[1]);
}

void
Renderer::setTransformAndTexScaleUniformsForDest(const UniformMap& uniforms, TileInfo* tileInfo)
{
  Vector2 usedSize = getUsedSizeFactor();

  Vector2 tileSize;
  Vector2 tilePosition;
  if (tileInfo == nullptr) {
    tileSize = Vector2::One();
    tilePosition = Vector2::Zero();
  } else {
    tileSize = tileInfo->size;
    tilePosition = tileInfo->position;
  }

  Matrix4 transform = Matrix4::Identity();
  transform.translate(
    -1.0 + tilePosition[0] / tileSize[0] * 2.0,
    -1.0 + tilePosition[1] / tileSize[1] * 2.0,
    0.0
  );
  transform.scale(2.0 * usedSize[0], 2.0 * usedSize[1], 1.0);
  transform.scale(1.0 / tileSize[0], 1.0 / tileSize[1], 1.0);
  glUniformMatrix4fv(uniforms.uTransform, 1, GL_FALSE, transform.c);
  glUniform2f(uniforms.uTexScale, usedSize[0], usedSize[1]);
}

void
Renderer::setTransformSTAndTexScaleUniformsForDest(const UniformMap& uniforms)
{
  Vector2 usedSize = getUsedSizeFactor();
  glUniform4f(uniforms.uTransformST, 2.0 * usedSize[0], 2.0 * usedSize[1], -1.0, -1.0);
  glUniform2f(uniforms.uTexScale, usedSize[0], usedSize[1]);
}

void
Renderer::setTransformUniform(const UniformMap& uniforms, int pass, int objectIndex)
{
  Matrix4 transform = computeTransform(pass, objectIndex);
  glUniformMatrix4fv(uniforms.uTransform, 1, GL_FALSE, transform.c);
}

void
Renderer::setTransformSTUniform(const UniformMap& uniforms, int objectIndex)
{
  // FIXME(pcwalton): Lossy conversion from a 4x4 matrix to an ST matrix is ugly and fragile.
  // Refactor.

  Matrix4 transform = computeTransform(0, objectIndex);

  glUniform4f(uniforms.uTransformST,
              transform[0],
              transform[5],
              transform[12],
              transform[13]);
}

void
Renderer::setTransformAffineUniforms(const UniformMap& uniforms, int objectIndex)
{
    // FIXME(pcwalton): Lossy conversion from a 4x4 matrix to an affine matrix is ugly and
    // fragile. Refactor.

    Matrix4 transform = computeTransform(0, objectIndex);

    glUniform4f(uniforms.uTransformST,
                transform[0],
                transform[5],
                transform[12],
                transform[13]);
    glUniform2f(uniforms.uTransformExt, transform[1], transform[4]);
}

void
Renderer::uploadPathColors(int objectCount)
{
  for (int objectIndex = 0; objectIndex < objectCount; objectIndex++) {
    // TODO(kip) - Eliminate copy:
    vector<__uint8_t> pathColors = pathColorsForObject(objectIndex);

    shared_ptr<PathfinderBufferTexture> pathColorsBufferTexture;
    if (objectIndex >= mPathColorsBufferTextures.size()) {
      pathColorsBufferTexture = make_shared<PathfinderBufferTexture>("uPathColors");
      mPathColorsBufferTextures[objectIndex] = pathColorsBufferTexture;
    } else {
      pathColorsBufferTexture = mPathColorsBufferTextures[objectIndex];
    }

    pathColorsBufferTexture->upload(pathColors);
  }
}

void
Renderer::uploadPathTransforms(int objectCount)
{
  for (int objectIndex = 0; objectIndex < objectCount; objectIndex++) {
    shared_ptr<PathTransformBuffers<vector<float>>> pathTransforms = pathTransformsForObject(objectIndex);

    shared_ptr<PathTransformBuffers<PathfinderBufferTexture>> pathTransformBufferTextures;
    if (objectIndex >= mPathTransformBufferTextures.size()) {
      pathTransformBufferTextures = make_shared<PathTransformBuffers<PathfinderBufferTexture>>(
        make_shared<PathfinderBufferTexture>("uPathTransformST"),
        make_shared<PathfinderBufferTexture>("uPathTransformExt"));
      mPathTransformBufferTextures[objectIndex] = pathTransformBufferTextures;
    } else {
      pathTransformBufferTextures = mPathTransformBufferTextures[objectIndex];
    }

    pathTransformBufferTextures->st->upload(*(pathTransforms->st));
    pathTransformBufferTextures->ext->upload(*(pathTransforms->ext));
  }
}

void
Renderer::setPathColorsUniform(int objectIndex, const UniformMap& uniforms, GLuint textureUnit)
{
  int meshIndex = meshIndexForObject(objectIndex);
  mPathColorsBufferTextures[meshIndex]->bind(uniforms, textureUnit);
}

void
Renderer::setEmboldenAmountUniform(int objectIndex, const UniformMap& uniforms)
{
  Vector2 emboldenAmount = getEmboldenAmount();
  glUniform2f(uniforms.uEmboldenAmount, emboldenAmount[0], emboldenAmount[1]);
}

int
Renderer::meshIndexForObject(int objectIndex)
{
  return objectIndex;
}

Range
Renderer::pathRangeForObject(int objectIndex)
{
  if (mMeshBuffers.size() == 0) {
    return Range(0, 0);
  }
  size_t length = mMeshBuffers[objectIndex]->bQuadVertexPositionPathRanges.size();
  return Range(1, length + 1);
}

void
Renderer::bindGammaLUT(Vector3 bgColor, GLuint textureUnit, const UniformMap& uniforms)
{
  glActiveTexture(GL_TEXTURE0 + textureUnit);
  glBindTexture(GL_TEXTURE_2D, mGammaLUTTexture);
  glUniform1i(uniforms.uGammaLUT, textureUnit);

  glUniform3f(uniforms.uBGColor, bgColor[0], bgColor[1], bgColor[2]);
}

void
Renderer::clearDestFramebuffer()
{
  Vector4 clearColor = getBackgroundColor();
  Vector2 destAllocatedSize = getDestAllocatedSize();
  glBindFramebuffer(GL_FRAMEBUFFER, getDestFramebuffer());
  glDepthMask(true);
  glViewport(0, 0, destAllocatedSize[0], destAllocatedSize[1]);
  glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
  glClearDepth(0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void
Renderer::clearForDirectRendering(int objectIndex)
{
  DirectRenderingMode renderingMode = mAntialiasingStrategy->getDirectRenderingMode();

  Vector4 clearColor = clearColorForObject(objectIndex);
  // FINDME!! KIP!! HACK!! -- RE-enable below, and make clear color optional...
  //
  // if (clearColor == Vector4::Zero()) {
  //   return;
  // }

  glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
  glClearDepth(0.0);
  glDepthMask(true);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

Matrix4
Renderer::getModelviewTransform(int pathIndex)
{
  return Matrix4::Identity();
}

Range
Renderer::instanceRangeForObject(int objectIndex)
{
  return Range(0, 1);
}

shared_ptr<PathTransformBuffers<vector<float>>>
Renderer::createPathTransformBuffers(int pathCount)
{
  pathCount += 1;
  return make_shared<PathTransformBuffers<vector<float>>>(
    make_shared<vector<float>>(pathCount * 4),
    make_shared<vector<float>>((pathCount + (pathCount & 1)) * 2));
}

void
Renderer::directlyRenderObject(int pass, int objectIndex)
{
  if (mMeshBuffers.size() == 0 || mMeshes.size() == 0) {
    return;
  }

  DirectRenderingMode renderingMode = mAntialiasingStrategy->getDirectRenderingMode();
  int objectCount = getObjectCount();

  Range instanceRange = instanceRangeForObject(objectIndex);
  if (instanceRange.isEmpty()) {
    return;
  }

  Range pathRange = pathRangeForObject(objectIndex);
  int meshIndex = meshIndexForObject(objectIndex);

  shared_ptr<PathfinderPackedMeshBuffers> meshes = mMeshBuffers[meshIndex];
  shared_ptr<PathfinderPackedMeshes> meshData = mMeshes[meshIndex];

  // Set up implicit cover state.
  glDepthFunc(GL_GREATER);
  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);
  glEnable(GL_CULL_FACE);

  // Set up the implicit cover interior VAO.
  int directInteriorProgramName = directInteriorProgramName(renderingMode);
  shared_ptr<PathfinderShaderProgram> directInteriorProgram = mRenderContext->shaderPrograms[directInteriorProgramName];
  if (mImplicitCoverInteriorVAO == 0) {
      mImplicitCoverInteriorVAO = mRenderContext->vertexArrayObjectExt
                                                   .createVertexArrayOES();
  }
  mRenderContext->vertexArrayObjectExt.bindVertexArrayOES(mImplicitCoverInteriorVAO);
  initImplicitCoverInteriorVAO(objectIndex, instanceRange, renderingMode);

  // Draw direct interior parts.
  if (renderingMode == drm_conservative) {
    setTransformAffineUniforms(directInteriorProgram.uniforms, objectIndex);
  } else {
    setTransformUniform(directInteriorProgram.uniforms, pass, objectIndex);
  }
  setFramebufferSizeUniform(directInteriorProgram.uniforms);
  setHintsUniform(directInteriorProgram.uniforms);
  setPathColorsUniform(objectIndex, directInteriorProgram.uniforms, 0);
  setEmboldenAmountUniform(objectIndex, directInteriorProgram.uniforms);
  pathTransformBufferTextures[meshIndex].st.bind(gl, directInteriorProgram.uniforms, 1);
  pathTransformBufferTextures[meshIndex].ext.bind(gl, directInteriorProgram.uniforms, 2);
  Range bQuadInteriorRange = getMeshIndexRange(meshes->bQuadVertexInteriorIndexPathRanges,
                                               pathRange);
  if (!getPathIDsAreInstanced()) {
    glDrawElements(GL_TRIANGLES,
                   bQuadInteriorRange.length,
                   GL_UNSIGNED_INT,
                   bQuadInteriorRange.start * UINT32_SIZE);
  } else {
      renderContext.instancedArraysExt
                   .drawElementsInstancedANGLE(gl.TRIANGLES,
                                               bQuadInteriorRange.length,
                                               gl.UNSIGNED_INT,
                                               0,
                                               instanceRange.length);
  }

  glDisable(GL_CULL_FACE);

  // Render curves, if applicable.
  if (renderingMode != drm_conservative) {
      // Set up direct curve state.
      glDepthMask(GL_FALSE);
      glEnable(GL_BLEND);
      glBlendEquation(GL_FUNC_ADD);
      glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

      // Set up the direct curve VAO.
      //
      // TODO(pcwalton): Cache these.
      ShaderName_t directCurveProgramName = directCurveProgramName();
      shared_ptr<PathfinderShaderProgram> directCurveProgram = mRenderContext->shaderPrograms[directCurveProgramName];
      if (mImplicitCoverCurveVAO == 0) {
          mImplicitCoverCurveVAO = mRenderContext->vertexArrayObjectExt
                                                    .createVertexArrayOES();
      }
      mRenderContext->vertexArrayObjectExt.bindVertexArrayOES(mImplicitCoverCurveVAO);
      initImplicitCoverCurveVAO(objectIndex, instanceRange);

      // Draw direct curve parts.
      setTransformUniform(directCurveProgram.uniforms, pass, objectIndex);
      setFramebufferSizeUniform(directCurveProgram.uniforms);
      setHintsUniform(directCurveProgram.uniforms);
      setPathColorsUniform(objectIndex, directCurveProgram.uniforms, 0);
      setEmboldenAmountUniform(objectIndex, directCurveProgram.uniforms);
      mPathTransformBufferTextures[meshIndex]
          .st
          .bind(gl, directCurveProgram.uniforms, 1);
      mPathTransformBufferTextures[meshIndex]
          .ext
          .bind(gl, directCurveProgram.uniforms, 2);
      Range coverCurveRange = getMeshIndexRange(meshes.bQuadVertexPositionPathRanges,
                                                pathRange);
      if (!getPathIDsAreInstanced()) {
          glDrawArrays(GL_TRIANGLES, coverCurveRange.start * 6, coverCurveRange.length * 6);
      } else {
          mRenderContext->instancedArraysExt
                       .drawArraysInstancedANGLE(GL_TRIANGLES,
                                                 0,
                                                 coverCurveRange.length * 6,
                                                 instanceRange.length);
      }
  }

  mRenderContext->vertexArrayObjectExt.bindVertexArrayOES(0);

  // Finish direct rendering. Right now, this performs compositing if necessary.
  mAntialiasingStrategy->finishDirectlyRenderingObject(*this, objectIndex);
}

void
Renderer::initGammaLUTTexture()
{
  GLuint gammaLUT = mRenderContext->gammaLUT;
  GLuint texture = glCreateTexture();
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, GL_LUMINANCE, GL_UNSIGNED_BYTE, gammaLUT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  mGammaLUTTexture = texture;
}

void
Renderer::initImplicitCoverCurveVAO(int objectIndex, Range instanceRange)
{
  if (mMeshBuffers.size() == 0) {
    return;
  }

  int meshIndex = meshIndexForObject(objectIndex);
  shared_ptr<PathfinderPackedMeshBuffers> meshes = mMeshBuffers[meshIndex];
  shared_ptr<PathfinderPackedMeshes> meshData = mMeshes[meshIndex];

  ShaderName_t directCurveProgramName = directCurveProgramName();
  shared_ptr<PathfinderShaderProgram> directCurveProgram = mRenderContext->shaderPrograms[directCurveProgramName];
  glUseProgram(directCurveProgram.program);
  glBindBuffer(GL_ARRAY_BUFFER, meshes->bQuadVertexPositions);
  glVertexAttribPointer(directCurveProgram->attributes.aPosition, 2, gl.FLOAT, false, 0, 0);
  glBindBuffer(gl.GL_ARRAY_BUFFER, mVertexIDVBO);
  glVertexAttribPointer(directCurveProgram->attributes.aVertexID, 1, gl.FLOAT, false, 0, 0);

  if (getPathIDsAreInstanced()) {
    glBindBuffer(GL_ARRAY_BUFFER, mInstancedPathIDVBO);
  } else {
    glBindBuffer(GL_ARRAY_BUFFER, meshes->bQuadVertexPositionPathIDs);
  }
  glVertexAttribPointer(directCurveProgram->attributes.aPathID,
                         1,
                         gL_UNSIGNED_SHORT,
                         GL_FALSE,
                         0,
                         instanceRange.start * UINT16_SIZE);
  if (getPathIDsAreInstanced()) {
      mRenderContext->instancedArraysExt
                   .vertexAttribDivisorANGLE(directCurveProgram.attributes.aPathID, 1);
  }

  glEnableVertexAttribArray(directCurveProgram->attributes.aPosition);
  glEnableVertexAttribArray(directCurveProgram->attributes.aVertexID);
  glEnableVertexAttribArray(directCurveProgram->attributes.aPathID);
}


void
Renderer::initImplicitCoverInteriorVAO(int objectIndex, Range instanceRange, DirectRenderingMode renderingMode)
{
  if (mMeshBuffers.size() == 0) {
    return;
  }

  int meshIndex = meshIndexForObject(objectIndex);
  shared_ptr<PathfinderPackedMeshBuffers> meshes = mMeshBuffers[meshIndex];

  ShaderName_t directInteriorProgramName = directInteriorProgramName(renderingMode);
  shared_ptr<PathfinderShaderProgram> directInteriorProgram = mRenderContext->shaderPrograms[directInteriorProgramName];
  glUseProgram(directInteriorProgram->program);
  glBindBuffer(GL_ARRAY_BUFFER, meshes->bQuadVertexPositions);
  glVertexAttribPointer(directInteriorProgram->attributes.aPosition,
                        2,
                        GL_FLOAT,
                        GL_FALSE,
                        0,
                        0);

  if (getPathIDsAreInstanced()) {
    glBindBuffer(GL_ARRAY_BUFFER, mInstancedPathIDVBO);
  } else {
    glBindBuffer(GL_ARRAY_BUFFER, meshes->bQuadVertexPositionPathIDs);
  }
  glVertexAttribPointer(directInteriorProgram->attributes.aPathID,
                        1,
                        GL_UNSIGNED_SHORT,
                        GL_FALSE,
                        0,
                        instanceRange.start * UINT16_SIZE);
  if (getPathIDsAreInstanced()) {
      renderContext->instancedArraysExt
                   .vertexAttribDivisorANGLE(directInteriorProgram.attributes.aPathID, 1);
  }

  if (directInteriorProgramName == shader_conservativeInterior) {
    glBindBuffer(GL_ARRAY_BUFFER, mVertexIDVBO);
    glVertexAttribPointer(directInteriorProgram->attributes.aVertexID,
                          1,
                          GL_FLOAT,
                          GL_FALSE,
                          0,
                          0);
  }

  glEnableVertexAttribArray(directInteriorProgram->attributes.aPosition);
  glEnableVertexAttribArray(directInteriorProgram->attributes.aPathID);
  if (directInteriorProgramName === shader_conservativeInterior) {
    glEnableVertexAttribArray(directInteriorProgram->attributes.aVertexID);
  }
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshes-=>bQuadVertexInteriorIndices);
}

void
Renderer::initInstancedPathIDVBO()
{
  __uint16_t *pathIDs = new __uint16_t[MAX_PATHS];
  for (int pathIndex = 0; pathIndex < MAX_PATHS; pathIndex++) {
   pathIDs[pathIndex] = pathIndex + 1;
  }

  mInstancedPathIDVBO = glCreateBuffer();
  glBindBuffer(GL_ARRAY_BUFFER, mInstancedPathIDVBO);
  glBufferData(GL_ARRAY_BUFFER, pathIDs, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  delete pathIDs;
}

void
Renderer::initVertexIDVBO()
{
  __uint16_t *vertexIDs = new float[MAX_VERTICES];
  for (let vertexID = 0; vertexID < MAX_VERTICES; vertexID++) {
    vertexIDs[vertexID] = vertexID;
  }

  mVertexIDVBO = glCreateBuffer();
  glBindBuffer(GL_ARRAY_BUFFER, mVertexIDVBO);
  glBufferData(GL_ARRAY_BUFFER, vertexIDs, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  delete vertexIDs;
}

kraken::Matrix4 Renderer::computeTransform(int pass, int objectIndex)
{
  Matrix4 transform;
  if (mAntialiasingStrategy) {
    transform = mAntialiasingStrategy->worldTransformForPass(this, pass);
  } else {
    transform = Matrix4::Identity();
  }

  transform *= getWorldTransform();
  transform *= getModelviewTransform(objectIndex);
  return transform;
}

Range getMeshIndexRange(const vector<Range>& indexRanges, Range pathRange)
{
  if (indexRanges.size() === 0) {
    return new Range(0, 0);
  }

  Range lastIndexRange = indexRanges.back();
  bool descending = indexRanges[0].start > lastIndexRange.start;

  pathRange = Range(pathRange.start - 1, pathRange.end - 1);

  int startIndex;
  if (pathRange.start >= indexRanges.size()) {
    startIndex = lastIndexRange.end;
  } else if (!descending) {
    startIndex = indexRanges[pathRange.start].start;
  } else {
    startIndex = indexRanges[pathRange.start].end;
  }

  int endIndex;
  if (descending) {
      endIndex = indexRanges[pathRange.end - 1].start;
  } else if (pathRange.end >= indexRanges.length) {
      endIndex = lastIndexRange.end;
  } else {
      endIndex = indexRanges[pathRange.end].start;
  }

  if (descending) {
      int tmp = startIndex;
      startIndex = endIndex;
      endIndex = tmp;
  }

  return Range(startIndex, endIndex);
}

} // namepsace pathfinder

