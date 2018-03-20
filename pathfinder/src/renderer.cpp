#include "renderer.h"

#include "resources.h"

#include "render-context.h"
#include "aa-strategy.h"

#include <assert.h>

namespace pathfinder {

Renderer::Renderer(std::shared_ptr<RenderContext> renderContext)
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

  mAntialiasingStrategy = std::make_shared<NoAAStrategy>(0, spaa_none);
  mAntialiasingStrategy->init(*this);
  mAntialiasingStrategy->setFramebufferSize(*this);
}

Renderer::~Renderer()
{
}

void
Renderer::attachMeshes(std::vector<std::shared_ptr<PathfinderPackedMeshes>>& meshes)
{
  assert(mAntialiasingStrategy);
  mMeshes = meshes;
  mMeshBuffers.clear();
  for (std::shared_ptr<PathfinderPackedMeshes>& m: meshes) {
    mMeshBuffers.push_back(std::move(std::make_shared<PathfinderPackedMeshBuffers>(m)));
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
  kraken::Vector2 destAllocatedSize = getDestAllocatedSize();
  glUniform2i(uniforms.uFramebufferSize,
              destAllocatedSize[0],
              destAllocatedSize[1]);
}

void
Renderer::setTransformAndTexScaleUniformsForDest(const UniformMap& uniforms, TileInfo* tileInfo)
{
  kraken::Vector2 usedSize = getUsedSizeFactor();

  kraken::Vector2 tileSize;
  kraken::Vector2 tilePosition;
  if (tileInfo == nullptr) {
    tileSize = kraken::Vector2::One();
    tilePosition = kraken::Vector2::Zero();
  } else {
    tileSize = tileInfo->size;
    tilePosition = tileInfo->position;
  }

  kraken::Matrix4 transform = kraken::Matrix4::Identity();
  transform.translate(
    -1.0 + tilePosition[0] / tileSize[0] * 2.0,
    -1.0 + tilePosition[1] / tileSize[1] * 2.0,
    0.0
  );
  transform.scale(2.0 * usedSize[0], 2.0 * usedSize[1], 1.0);
  transform.scale(1.0 / tileSize[0], 1.0 / tileSize[1], 1.0);
  glUniformMatrix4fv(uniforms.uTransform, false, &transform.c);
  glUniform2f(uniforms.uTexScale, usedSize[0], usedSize[1]);
}

void
Renderer::setTransformSTAndTexScaleUniformsForDest(const UniformMap& uniforms)
{
  kraken::Vector2 usedSize = getUsedSizeFactor();
  glUniform4f(uniforms.uTransformST, 2.0 * usedSize[0], 2.0 * usedSize[1], -1.0, -1.0);
  glUniform2f(uniforms.uTexScale, usedSize[0], usedSize[1]);
}

void
Renderer::setTransformUniform(const UniformMap& uniforms, int pass, int objectIndex)
{
  kraken::Matrix4 transform = computeTransform(pass, objectIndex);
  glUniformMatrix4fv(uniforms.uTransform, false, &transform.c);
}

void
Renderer::setTransformSTUniform(const UniformMap& uniforms, int objectIndex)
{
  // FIXME(pcwalton): Lossy conversion from a 4x4 matrix to an ST matrix is ugly and fragile.
  // Refactor.

  kraken::Matrix4 transform = computeTransform(0, objectIndex);

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

    kraken::Matrix4 transform = computeTransform(0, objectIndex);

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
    __uint8_t* pathColors = pathColorsForObject(objectIndex);

    std::shared_ptr<PathfinderBufferTexture> pathColorsBufferTexture;
    if (objectIndex >= mPathColorsBufferTextures.size()) {
      pathColorsBufferTexture = std::make_shared<PathfinderBufferTexture>("uPathColors");
      mPathColorsBufferTextures[objectIndex] = pathColorsBufferTexture;
    } else {
      pathColorsBufferTexture = pathColorsBufferTextures[objectIndex];
    }

    pathColorsBufferTexture.upload(pathColors);
  }
}

void
Renderer::uploadPathTransforms(int objectCount)
{
  for (int objectIndex = 0; objectIndex < objectCount; objectIndex++) {
    std::vector<float> pathTransforms = pathTransformsForObject(objectIndex);

    std::shared_ptr<PathTransformBuffers<PathfinderBufferTexture>> pathTransformBufferTextures;
    if (objectIndex >= mPathTransformBufferTextures.size()) {
      pathTransformBufferTextures = std::make_shared<PathTransformBuffers<PathfinderBufferTexture>>(
        std::make_shared<PathfinderBufferTexture>("uPathTransformST"),
        std::make_shared<PathfinderBufferTexture>("uPathTransformExt"));
      this.pathTransformBufferTextures[objectIndex] = pathTransformBufferTextures;
    } else {
      pathTransformBufferTextures = this.pathTransformBufferTextures[objectIndex];
    }

    pathTransformBufferTextures.st.upload(pathTransforms.st);
    pathTransformBufferTextures.ext.upload(pathTransforms.ext);
  }
}

void
Renderer::setPathColorsUniform(int objectIndex, const UniformMap& uniforms, GLuint textureUnit)
{
  int meshIndex = meshIndexForObject(objectIndex);
  mPathColorsBufferTextures[meshIndex].bind(uniforms, textureUnit);
}

void
Renderer::setEmboldenAmountUniform(int objectIndex, const UniformMap& uniforms)
{
  kraken::Vector2 emboldenAmount = getEmboldenAmount();
  glUniform2f(uniforms.uEmboldenAmount, emboldenAmount[0], emboldenAmount[1]);
}

int
Renderer::meshIndexForObject(int objectIndex)
{
  return objectIndex;
}

Range
Renderer::pathRangeForObject(int objectIndex);
  if (mMeshBuffers.size() == 0) {
    return Range(0, 0);
  }
  std::Vector<Range>& bVertexPathRanges = mMeshBuffers[objectIndex].bQuadVertexPositionPathRanges;
  return Range(1, bVertexPathRanges.length + 1);
}

void
Renderer::bindGammaLUT(kraken::Vector3 bgColor, GLuint textureUnit, const UniformMap& uniforms)
{
  glActiveTexture(GL_TEXTURE0 + textureUnit);
  glBindTexture(GL_TEXTURE_2D, mGammaLUTTexture);
  glUniform1i(uniforms.uGammaLUT, textureUnit);

  glUniform3f(uniforms.uBGColor, bgColor[0], bgColor[1], bgColor[2]);
}

void
Renderer::clearDestFramebuffer()
{
  kraken::Vector4 clearColor = getBackgroundColor();
  kraken::Vector2 destAllocatedSize = getDestAllocatedSize();
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
  DirectRenderingMode mAntialiasingStrategy.getDirectRenderingMode();

  kraken::Vector4 clearColor = clearColorForObject(objectIndex);
  // FINDME!! KIP!! HACK!! -- RE-enable below, and make clear color optional...
  //
  // if (clearColor == kraken::Vector4::Zero()) {
  //   return;
  // }

  glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
  glClearDepth(0.0);
  glDepthMask(true);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

kraken::Matrix4
Renderer::getModelviewTransform(int pathIndex)
{
  return kraken::Matrix4::Identity();
}

Range
Renderer::instanceRangeForObject(int objectIndex)
{
  return new Range(0, 1);
}

std::shared_ptr<PathTransformBuffers<std::vector<float>>>
Renderer::createPathTransformBuffers(int pathCount)
{
  pathCount += 1;
  return std::make_shared<PathTransformBuffers<std::vector<float>>>(
    std::make_shared<std::vector<float>>(pathCount * 4),
    std::make_shared<std::vector<float>>((pathCount + (pathCount & 1)) * 2));
}

void
Renderer::directlyRenderObject(int pass, int objectIndex)
{
  if (mMeshBuffers.size() == 0 || mMeshes.size() == 0) {
    return;
  }

  DirectRenderingMode renderingMode = mAntialiasingStrategy.getDirectRenderingMode();
  int objectCount = getObjectCount();

  Range instanceRange = instanceRangeForObject(objectIndex);
  if (instanceRange.isEmpty) {
    return;
  }

  Range pathRange = pathRangeForObject(objectIndex);
  int meshIndex = meshIndexForObject(objectIndex);

  std::shared_ptr<PathfinderPackedMeshBuffers> meshes = mMeshBuffers[meshIndex];
  std::shared_ptr<PathfinderPackedMeshes> meshData = mMeshes[meshIndex];

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
  const directInteriorProgram = renderContext.shaderPrograms[directInteriorProgramName];
  if (this.implicitCoverInteriorVAO == null) {
      this.implicitCoverInteriorVAO = renderContext.vertexArrayObjectExt
                                                   .createVertexArrayOES();
  }
  renderContext.vertexArrayObjectExt.bindVertexArrayOES(this.implicitCoverInteriorVAO);
  this.initImplicitCoverInteriorVAO(objectIndex, instanceRange, renderingMode);

  // Draw direct interior parts.
  if (renderingMode === 'conservative')
      this.setTransformAffineUniforms(directInteriorProgram.uniforms, objectIndex);
  else
      this.setTransformUniform(directInteriorProgram.uniforms, pass, objectIndex);
  this.setFramebufferSizeUniform(directInteriorProgram.uniforms);
  this.setHintsUniform(directInteriorProgram.uniforms);
  this.setPathColorsUniform(objectIndex, directInteriorProgram.uniforms, 0);
  this.setEmboldenAmountUniform(objectIndex, directInteriorProgram.uniforms);
  this.pathTransformBufferTextures[meshIndex].st.bind(gl, directInteriorProgram.uniforms, 1);
  this.pathTransformBufferTextures[meshIndex]
      .ext
      .bind(gl, directInteriorProgram.uniforms, 2);
  const bQuadInteriorRange = getMeshIndexRange(meshes.bQuadVertexInteriorIndexPathRanges,
                                               pathRange);
  if (!this.pathIDsAreInstanced) {
      gl.drawElements(gl.TRIANGLES,
                      bQuadInteriorRange.length,
                      gl.UNSIGNED_INT,
                      bQuadInteriorRange.start * UINT32_SIZE);
  } else {
      renderContext.instancedArraysExt
                   .drawElementsInstancedANGLE(gl.TRIANGLES,
                                               bQuadInteriorRange.length,
                                               gl.UNSIGNED_INT,
                                               0,
                                               instanceRange.length);
  }

  gl.disable(gl.CULL_FACE);

  // Render curves, if applicable.
  if (renderingMode !== 'conservative') {
      // Set up direct curve state.
      gl.depthMask(false);
      gl.enable(gl.BLEND);
      gl.blendEquation(gl.FUNC_ADD);
      gl.blendFuncSeparate(gl.ONE, gl.ONE_MINUS_SRC_ALPHA, gl.ONE, gl.ONE);

      // Set up the direct curve VAO.
      //
      // TODO(pcwalton): Cache these.
      const directCurveProgramName = this.directCurveProgramName();
      const directCurveProgram = renderContext.shaderPrograms[directCurveProgramName];
      if (this.implicitCoverCurveVAO == null) {
          this.implicitCoverCurveVAO = renderContext.vertexArrayObjectExt
                                                    .createVertexArrayOES();
      }
      renderContext.vertexArrayObjectExt.bindVertexArrayOES(this.implicitCoverCurveVAO);
      this.initImplicitCoverCurveVAO(objectIndex, instanceRange);

      // Draw direct curve parts.
      this.setTransformUniform(directCurveProgram.uniforms, pass, objectIndex);
      this.setFramebufferSizeUniform(directCurveProgram.uniforms);
      this.setHintsUniform(directCurveProgram.uniforms);
      this.setPathColorsUniform(objectIndex, directCurveProgram.uniforms, 0);
      this.setEmboldenAmountUniform(objectIndex, directCurveProgram.uniforms);
      this.pathTransformBufferTextures[meshIndex]
          .st
          .bind(gl, directCurveProgram.uniforms, 1);
      this.pathTransformBufferTextures[meshIndex]
          .ext
          .bind(gl, directCurveProgram.uniforms, 2);
      const coverCurveRange = getMeshIndexRange(meshes.bQuadVertexPositionPathRanges,
                                                pathRange);
      if (!this.pathIDsAreInstanced) {
          gl.drawArrays(gl.TRIANGLES, coverCurveRange.start * 6, coverCurveRange.length * 6);
      } else {
          renderContext.instancedArraysExt
                       .drawArraysInstancedANGLE(gl.TRIANGLES,
                                                 0,
                                                 coverCurveRange.length * 6,
                                                 instanceRange.length);
      }
  }

  renderContext.vertexArrayObjectExt.bindVertexArrayOES(null);

  // Finish direct rendering. Right now, this performs compositing if necessary.
  antialiasingStrategy.finishDirectlyRenderingObject(this, objectIndex);
}

} // namepsace pathfinder

