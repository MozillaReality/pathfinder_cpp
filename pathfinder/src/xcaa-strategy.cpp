// pathfinder/src/xcaa-strategy.cpp
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include "xcaa-strategy.h"

#include "renderer.h"
#include "gl-utils.h"
#include "shader-loader.h"
#include "meshes.h"
#include <kraken-math.h>
#include <memory>
#include <assert.h>

using namespace std;
using namespace kraken;

namespace pathfinder {

XCAAStrategy::XCAAStrategy(int aLevel, SubpixelAAType aSubpixelAA)
  : AntialiasingStrategy(aSubpixelAA)
  , patchVertexBuffer(0)
  , patchIndexBuffer(0)
  , resolveVAO(0)
  , aaAlphaTexture(0)
  , aaDepthTexture(0)
  , aaFramebuffer(0)
{
  supersampledFramebufferSize.init();
  destFramebufferSize.init();
}

void
XCAAStrategy::attachMeshes(RenderContext& renderContext, Renderer& renderer)
{
  createResolveVAO(renderContext, renderer);
  pathBoundsBufferTextures.clear();

  GLDEBUG(glCreateBuffers(1, &patchVertexBuffer));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, patchVertexBuffer));
  GLDEBUG(glBufferData(GL_ARRAY_BUFFER, PATCH_VERTICES_SIZE, (GLvoid*)PATCH_VERTICES, GL_STATIC_DRAW));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, 0));

  GLDEBUG(glCreateBuffers(1, &patchIndexBuffer));
  GLDEBUG(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, patchIndexBuffer));
  GLDEBUG(glBufferData(GL_ELEMENT_ARRAY_BUFFER, MCAA_PATCH_INDICES_SIZE, (GLvoid*)MCAA_PATCH_INDICES, GL_STATIC_DRAW));
  GLDEBUG(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

void
XCAAStrategy::setFramebufferSize(Renderer& renderer)
{
  destFramebufferSize = renderer.getDestAllocatedSize();
  supersampledFramebufferSize = Vector2i::Create(destFramebufferSize.x * getSupersampleScale().x,
                                         destFramebufferSize.y * getSupersampleScale().y);

  initAAAlphaFramebuffer(renderer);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void
XCAAStrategy::finishAntialiasingObject(Renderer& renderer, int objectIndex)
{
  initResolveFramebufferForObject(renderer, objectIndex);

  if (!usesAAFramebuffer(renderer)) {
    return;
  }

  Vector2i usedSize = supersampledUsedSize(renderer);
  GLDEBUG(glScissor(0, 0, usedSize[0], usedSize[1]));
  GLDEBUG(glEnable(GL_SCISSOR_TEST));

  // Clear out the color and depth textures.
  GLDEBUG(glClearColor(1.0, 1.0, 1.0, 1.0));
  GLDEBUG(glClearDepth(0.0));
  GLDEBUG(glDepthMask(GL_TRUE));
  GLDEBUG(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void
XCAAStrategy::finishDirectlyRenderingObject(Renderer& renderer, int objectIndex)
{
  // TODO(pcwalton)
};

void
XCAAStrategy::antialiasObject(Renderer& renderer, int objectIndex)
{
  // Perform early preparations.
  createPathBoundsBufferTextureForObjectIfNecessary(renderer, objectIndex);

  // Set up antialiasing.
  prepareAA(renderer);

  // Clear.
  clearForAA(renderer);
}

void
XCAAStrategy::resolveAAForObject(Renderer& renderer, int objectIndex)
{
  if (!usesAAFramebuffer(renderer)) {
    return;
  }

  PathfinderShaderProgram& resolveProgram = getResolveProgram(renderer);

  // Set state for XCAA resolve.
  Vector2i usedSize = renderer.getDestUsedSize();
  GLDEBUG(glScissor(0, 0, usedSize[0], usedSize[1]));
  GLDEBUG(glEnable(GL_SCISSOR_TEST));
  setDepthAndBlendModeForResolve();

  // Clear out the resolve buffer, if necessary.
  clearForResolve(renderer);

  // Resolve.
  GLDEBUG(glUseProgram(resolveProgram.getProgram()));
  // was renderContext.vertexArrayObjectExt
  GLDEBUG(glBindVertexArray(resolveVAO));
  GLDEBUG(glUniform2i(resolveProgram.getUniforms()["uFramebufferSize"],
    destFramebufferSize[0],
    destFramebufferSize[1]));
  GLDEBUG(glActiveTexture(GL_TEXTURE0));
  GLDEBUG(glBindTexture(GL_TEXTURE_2D, aaAlphaTexture));
  GLDEBUG(glUniform1i(resolveProgram.getUniforms()["uAAAlpha"], 0));
  GLDEBUG(glUniform2i(resolveProgram.getUniforms()["uAAAlphaDimensions"],
    supersampledFramebufferSize[0],
    supersampledFramebufferSize[1]));
  if (renderer.getBGColor() != Vector4::Zero()) {
    GLDEBUG(glUniform4fv(resolveProgram.getUniforms()["uBGColor"], sizeof(float) * 4, renderer.getBGColor().c));
  }
  if (renderer.getFGColor() != Vector4::Zero()) {
    GLDEBUG(glUniform4fv(resolveProgram.getUniforms()["uFGColor"], sizeof(float) * 4, renderer.getFGColor().c));
  }
  renderer.setTransformSTAndTexScaleUniformsForDest(resolveProgram.getUniforms());
  setSubpixelAAKernelUniform(renderer, resolveProgram.getUniforms());
  setAdditionalStateForResolveIfNecessary(renderer, resolveProgram, 1);
  GLDEBUG(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0));
  // was vertexArrayObjectExt.bindVertexArrayOES
  GLDEBUG(glBindVertexArray(0));
}

Matrix4
XCAAStrategy::getTransform() const
{
  return Matrix4::Identity();
}


Vector2i
XCAAStrategy::supersampledUsedSize(Renderer& renderer)
{
  return Vector2i::Create(renderer.getDestUsedSize().x * getSupersampleScale().x,
                          renderer.getDestUsedSize().y * getSupersampleScale().y);
}

void
XCAAStrategy::prepareAA(Renderer& renderer)
{
  // Set state for antialiasing.
  Vector2i usedSize = supersampledUsedSize(renderer);
  if (usesAAFramebuffer(renderer)) {
    GLDEBUG(glBindFramebuffer(GL_FRAMEBUFFER, aaFramebuffer));
  }
  GLDEBUG(glViewport(0,
             0,
             supersampledFramebufferSize[0],
             supersampledFramebufferSize[1]));
  GLDEBUG(glScissor(0, 0, usedSize[0], usedSize[1]));
  GLDEBUG(glEnable(GL_SCISSOR_TEST));
}

void
XCAAStrategy::setAAState(Renderer& renderer)
{
  Vector2i usedSize = supersampledUsedSize(renderer);
  if (usesAAFramebuffer(renderer)) {
    GLDEBUG(glBindFramebuffer(GL_FRAMEBUFFER, aaFramebuffer));
  }
  GLDEBUG(glViewport(0,
             0,
             supersampledFramebufferSize[0],
             supersampledFramebufferSize[1]));
  GLDEBUG(glScissor(0, 0, usedSize[0], usedSize[1]));
  GLDEBUG(glEnable(GL_SCISSOR_TEST));

  setAADepthState(renderer);
}

void
XCAAStrategy::setAAUniforms(Renderer& renderer, UniformMap& uniforms, int objectIndex) {
  switch (getTransformType()) {
  case tt_affine:
    renderer.setTransformAffineUniforms(uniforms, 0);
    break;
  case tt_3d:
    renderer.setTransformUniform(uniforms, 0, 0);
    break;
  }

  GLDEBUG(glUniform2i(uniforms["uFramebufferSize"],
              supersampledFramebufferSize[0],
              supersampledFramebufferSize[1]));
  renderer.getPathTransformBufferTextures()[0]->ext->bind(uniforms, 0);
  renderer.getPathTransformBufferTextures()[0]->st->bind(uniforms, 1);
  pathBoundsBufferTextures[objectIndex]->bind(uniforms, 2);
  renderer.setHintsUniform(uniforms);
  renderer.bindAreaLUT(4, uniforms);
}

void
XCAAStrategy::setDepthAndBlendModeForResolve()
{
  GLDEBUG(glDisable(GL_DEPTH_TEST));
  GLDEBUG(glDisable(GL_BLEND));
}

void
XCAAStrategy::initResolveFramebufferForObject(Renderer& renderer, int objectIndex) {
  GLDEBUG(glBindFramebuffer(GL_FRAMEBUFFER, renderer.getDestFramebuffer()));
  GLDEBUG(glViewport(0, 0, destFramebufferSize[0], destFramebufferSize[1]));
  GLDEBUG(glDisable(GL_SCISSOR_TEST));
}

void
XCAAStrategy::initAAAlphaFramebuffer(Renderer& renderer)
{
  if (!getMightUseAAFramebuffer()) {
    GLDEBUG(glDeleteTextures(1, &aaAlphaTexture));
    aaAlphaTexture = 0;
    GLDEBUG(glDeleteTextures(1, &aaDepthTexture));
    aaDepthTexture = 0;
    GLDEBUG(glDeleteTextures(1, &aaFramebuffer));
    aaFramebuffer = 0;
    return;
  }
  GLDEBUG(glCreateTextures(GL_TEXTURE_2D, 1, &aaAlphaTexture));

  GLDEBUG(glActiveTexture(GL_TEXTURE0));
  GLDEBUG(glBindTexture(GL_TEXTURE_2D, aaAlphaTexture));
  GLDEBUG(glTexImage2D(GL_TEXTURE_2D,
    0,
    GL_RGB,
    supersampledFramebufferSize[0],
    supersampledFramebufferSize[1],
    0,
    GL_RGB,
    GL_HALF_FLOAT, // was renderContext.textureHalfFloatExt.GL_HALF_FLOAT_OES
    0));
  setTextureParameters(GL_NEAREST);

  aaDepthTexture = createFramebufferDepthTexture(supersampledFramebufferSize);
  aaFramebuffer = createFramebuffer(aaAlphaTexture, aaDepthTexture);
}

void
XCAAStrategy::createPathBoundsBufferTextureForObjectIfNecessary(Renderer& renderer, int objectIndex)
{
  float* pathBounds = renderer.pathBoundingRects(objectIndex);
  int pathBoundsLength = renderer.pathBoundingRectsLength(objectIndex);

  if (pathBoundsBufferTextures[objectIndex] == nullptr) {
    pathBoundsBufferTextures[objectIndex] =
      make_unique<PathfinderBufferTexture>("uPathBounds");
  }

  pathBoundsBufferTextures[objectIndex]->upload(pathBounds, pathBoundsLength);
}

void
XCAAStrategy::createResolveVAO(RenderContext& renderContext, Renderer& renderer)
{
  if (!usesResolveProgram(renderer)) {
    return;
  }

  PathfinderShaderProgram& resolveProgram = getResolveProgram(renderer);

  // was vertexArrayObjectExt.createVertexArrayOES
  GLDEBUG(glCreateVertexArrays(1, &resolveVAO));
  // was vertexArrayObjectExt.bindVertexArrayOES
  GLDEBUG(glBindVertexArray(resolveVAO));

  GLDEBUG(glUseProgram(resolveProgram.getProgram()));
  renderContext.initQuadVAO(resolveProgram.getAttributes());

  // was vertexArrayObjectExt.bindVertexArrayOES
  GLDEBUG(glBindVertexArray(0));
}

TransformType
MCAAStrategy::getTransformType() const
{
  return tt_affine;
}
bool
MCAAStrategy::getMightUseAAFramebuffer() const
{
  return true;
}
bool
MCAAStrategy::usesAAFramebuffer(Renderer& renderer)
{
  return !renderer.getIsMulticolor();
}
bool
MCAAStrategy::usesResolveProgram(Renderer& renderer)
{
  return !renderer.getIsMulticolor();
}

void
MCAAStrategy::attachMeshes(RenderContext& renderContext, Renderer& renderer)
{
  XCAAStrategy::attachMeshes(renderContext, renderer);
  // was vertexArrayObjectExt.createVertexArrayOES();
  GLDEBUG(glCreateVertexArrays(1, &mVAO));
}

void
MCAAStrategy::antialiasObject(Renderer& renderer, int objectIndex)
{
  XCAAStrategy::antialiasObject(renderer, objectIndex);
  PathfinderShaderProgram& shaderProgram = edgeProgram(renderer);
  antialiasEdgesOfObjectWithProgram(renderer, objectIndex, shaderProgram);
}

PathfinderShaderProgram&
MCAAStrategy::getResolveProgram(Renderer& renderer)
{
  shared_ptr<RenderContext> renderContext = renderer.getRenderContext();
  assert(!renderer.getIsMulticolor());
  if (mSubpixelAA != saat_none) {
    return *renderContext->getShaderManager().getProgram(program_xcaaMonoSubpixelResolve);
  }
  return *renderContext->getShaderManager().getProgram(program_xcaaMonoResolve);
}

void
MCAAStrategy::clearForAA(Renderer& renderer)
{
  if (!usesAAFramebuffer(renderer)) {
    return;
  }

  GLDEBUG(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
  GLDEBUG(glClearDepth(0.0f));
  GLDEBUG(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void
MCAAStrategy::setAADepthState(Renderer& renderer)
{

  if (getDirectRenderingMode() != drm_conservative) {
    GLDEBUG(glDisable(GL_DEPTH_TEST));
    return;
  }

  GLDEBUG(glDepthFunc(GL_GREATER));
  GLDEBUG(glDepthMask(GL_FALSE));
  GLDEBUG(glEnable(GL_DEPTH_TEST));
  GLDEBUG(glDisable(GL_CULL_FACE));
}

void
MCAAStrategy::clearForResolve(Renderer& renderer)
{
  if (!renderer.getIsMulticolor()) {
    GLDEBUG(glClearColor(0.0f, 0.0f, 0.0, 1.0f));
    GLDEBUG(glClear(GL_COLOR_BUFFER_BIT));
  }
}


void
MCAAStrategy::setBlendModeForAA(Renderer& renderer)
{
  if (renderer.getIsMulticolor()) {
    GLDEBUG(glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE));
  } else {
    GLDEBUG(glBlendFunc(GL_ONE, GL_ONE));
  }
  GLDEBUG(glBlendEquation(GL_FUNC_ADD));
  GLDEBUG(glEnable(GL_BLEND));
}

void MCAAStrategy::prepareAA(Renderer& renderer)
{
  XCAAStrategy::prepareAA(renderer);

  setBlendModeForAA(renderer);
}

void
MCAAStrategy::initVAOForObject(Renderer& renderer, int objectIndex)
{
  if (!renderer.getMeshesAttached()) {
    return;
  }
  Range pathRange = renderer.pathRangeForObject(objectIndex);
  int meshIndex = renderer.meshIndexForObject(objectIndex);

  PathfinderShaderProgram& shaderProgram = edgeProgram(renderer);
  std::map<std::string, GLint>& attributes = shaderProgram.getAttributes();

  // FIXME(pcwalton): Refactor.
  // was vertexArrayObjectExt.bindVertexArrayOES
  GLDEBUG(glBindVertexArray(mVAO));

  std::vector<Range>& bBoxRanges = renderer.getMeshes()[meshIndex]->bBoxPathRanges;
  off_t offset = calculateStartFromIndexRanges(pathRange, bBoxRanges);

  GLDEBUG(glUseProgram(shaderProgram.getProgram()));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, renderer.getRenderContext()->quadPositionsBuffer()));
  GLDEBUG(glVertexAttribPointer(attributes["aTessCoord"], 2, GL_FLOAT, GL_FALSE, FLOAT32_SIZE * 2, 0));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, renderer.getMeshBuffers()[meshIndex]->bBoxes));
  GLDEBUG(glVertexAttribPointer(attributes["aRect"],
    4,
    GL_FLOAT,
    false,
    sizeof(float) * 20,
    (void *)(sizeof(float) * 0 + offset * sizeof(float) * 20)));
  GLDEBUG(glVertexAttribPointer(attributes["aUV"],
    4,
    GL_FLOAT,
    false,
    sizeof(float) * 20,
    (void *)(sizeof(float) * 4 + offset * sizeof(float) * 20)));
  GLDEBUG(glVertexAttribPointer(attributes["aDUVDX"],
    4,
    GL_FLOAT,
    false,
    sizeof(float) * 20,
    (void *)(sizeof(float) * 8 + offset * sizeof(float) * 20)));
  GLDEBUG(glVertexAttribPointer(attributes["aDUVDY"],
    4,
    GL_FLOAT,
    false,
    sizeof(float) * 20,
    (void *)(sizeof(float) * 12 + offset * sizeof(float) * 20)));
  GLDEBUG(glVertexAttribPointer(attributes["aSignMode"],
    4,
    GL_FLOAT,
    false,
    sizeof(float) * 20,
    (void *)(sizeof(float) * 16 + offset * sizeof(float) * 20)));

  GLDEBUG(glEnableVertexAttribArray(attributes["aTessCoord"]));
  GLDEBUG(glEnableVertexAttribArray(attributes["aRect"]));
  GLDEBUG(glEnableVertexAttribArray(attributes["aUV"]));
  GLDEBUG(glEnableVertexAttribArray(attributes["aDUVDX"]));
  GLDEBUG(glEnableVertexAttribArray(attributes["aDUVDY"]));
  GLDEBUG(glEnableVertexAttribArray(attributes["aSignMode"]));

  // was instancedArraysExt.vertexAttribDivisorANGLE
  GLDEBUG(glVertexAttribDivisor(attributes["aRect"], 1));
  GLDEBUG(glVertexAttribDivisor(attributes["aUV"], 1));
  GLDEBUG(glVertexAttribDivisor(attributes["aDUVDX"], 1));
  GLDEBUG(glVertexAttribDivisor(attributes["aDUVDY"], 1));
  GLDEBUG(glVertexAttribDivisor(attributes["aSignMode"], 1));

  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, renderer.getMeshBuffers()[meshIndex]->bBoxPathIDs));
  GLDEBUG(glVertexAttribPointer(attributes["aPathID"],
    1,
    GL_UNSIGNED_SHORT,
    false,
    UINT16_SIZE,
    (void *)(offset * sizeof(__uint16_t)));
  GLDEBUG(glEnableVertexAttribArray(attributes["aPathID"])));
  // was instancedArraysExt.vertexAttribDivisorANGLE
  GLDEBUG(glVertexAttribDivisor(attributes["aPathID"], 1));

  GLDEBUG(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer.getRenderContext()->quadElementsBuffer()));

  // was vertexArrayObjectExt.bindVertexArrayOES
  GLDEBUG(glBindVertexArray(0));
}

PathfinderShaderProgram&
MCAAStrategy::edgeProgram(Renderer& renderer)
{
  return *renderer.getRenderContext()->getShaderManager().getProgram(program_mcaa);
}

void
MCAAStrategy::antialiasEdgesOfObjectWithProgram(Renderer& renderer,
                                                int objectIndex,
                                                PathfinderShaderProgram& shaderProgram)
{
  if (!renderer.getMeshesAttached()) {
    return;
  }

  Range pathRange = renderer.pathRangeForObject(objectIndex);
  int meshIndex = renderer.meshIndexForObject(objectIndex);

  initVAOForObject(renderer, objectIndex);

  GLDEBUG(glUseProgram(shaderProgram.getProgram()));
  UniformMap& uniforms = shaderProgram.getUniforms();
  setAAUniforms(renderer, uniforms, objectIndex);

  // FIXME(pcwalton): Refactor.
  // was vertexArrayObjectExt.bindVertexArrayOES
  GLDEBUG(glBindVertexArray(mVAO));

  setBlendModeForAA(renderer);
  setAADepthState(renderer);

  GLDEBUG(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderer.getRenderContext()->quadElementsBuffer()));

  std::vector<Range>& bBoxRanges = renderer.getMeshes()[meshIndex]->bBoxPathRanges;
  int count = calculateCountFromIndexRanges(pathRange, bBoxRanges);

  // was instancedArraysExt.drawElementsInstancedANGLE
  GLDEBUG(glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0, count));

  // was vertexArrayObjectExt.bindVertexArrayOES
  GLDEBUG(glBindVertexArray(0));
  GLDEBUG(glDisable(GL_DEPTH_TEST));
  GLDEBUG(glDisable(GL_CULL_FACE));
}

DirectRenderingMode
MCAAStrategy::getDirectRenderingMode() const
{
  // FIXME(pcwalton): Only in multicolor mode?
  return drm_conservative;
}

void
MCAAStrategy::setAAUniforms(Renderer& renderer, UniformMap& uniforms, int objectIndex)
{
  XCAAStrategy::setAAUniforms(renderer, uniforms, objectIndex);
  renderer.setPathColorsUniform(0, uniforms, 3);

  GLDEBUG(glUniform1i(uniforms["uMulticolor"], renderer.getIsMulticolor() ? 1 : 0));
}


DirectRenderingMode
StencilAAAStrategy::getDirectRenderingMode() const
{
  return drm_none;
}

TransformType
StencilAAAStrategy::getTransformType() const
{
  return tt_affine;
}

bool
StencilAAAStrategy::getMightUseAAFramebuffer() const
{
  return true;
}

void
StencilAAAStrategy::attachMeshes(RenderContext& renderContext, Renderer& renderer)
{
  XCAAStrategy::attachMeshes(renderContext, renderer);
  createVAO(renderer);
}

void
StencilAAAStrategy::antialiasObject(Renderer& renderer, int objectIndex)
{
  XCAAStrategy::antialiasObject(renderer, objectIndex);

  if (renderer.getMeshes().size() == 0) {
    return;
  }

  // Antialias.
  setAAState(renderer);
  setBlendModeForAA(renderer);

  PathfinderShaderProgram& program = *renderer.getRenderContext()->getShaderManager().getProgram(program_stencilAAA);
  GLDEBUG(glUseProgram(program.getProgram()));
  UniformMap& uniforms = program.getUniforms();
  setAAUniforms(renderer, uniforms, objectIndex);

  // was vertexArrayObjectExt.bindVertexArrayOES
  GLDEBUG(glBindVertexArray(mVAO));

  // FIXME(pcwalton): Only render the appropriate instances.
  int count = renderer.getMeshes()[0]->stencilSegmentsCount();
  for (int side = 0; side < 2; side++) {
    GLDEBUG(glUniform1i(uniforms["uSide"], side));
    // was instancedArraysExt.drawElementsInstancedANGLE
    GLDEBUG(glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0, count));
  }

  // was vertexArrayObjectExt.bindVertexArrayOES
  GLDEBUG(glBindVertexArray(0));
}

bool
StencilAAAStrategy::usesAAFramebuffer(Renderer& renderer)
{
  return true;
}

void
StencilAAAStrategy::clearForAA(Renderer& renderer)
{
  GLDEBUG(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
  GLDEBUG(glClearDepth(0.0f));
  GLDEBUG(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

bool
StencilAAAStrategy::usesResolveProgram(Renderer& renderer)
{
  return true;
}

PathfinderShaderProgram&
StencilAAAStrategy::getResolveProgram(Renderer& renderer)
{
  RenderContext& renderContext = *renderer.getRenderContext();

  if (mSubpixelAA != saat_none) {
    return *renderContext.getShaderManager().getProgram(program_xcaaMonoSubpixelResolve);
  }
  return *renderContext.getShaderManager().getProgram(program_xcaaMonoResolve);
}

void
StencilAAAStrategy::setAADepthState(Renderer& renderer)
{
  GLDEBUG(glDisable(GL_DEPTH_TEST));
  GLDEBUG(glDisable(GL_CULL_FACE));
}

void
StencilAAAStrategy::setAAUniforms(Renderer& renderer, UniformMap& uniforms, int objectIndex)
{
  XCAAStrategy::setAAUniforms(renderer, uniforms, objectIndex);
  renderer.setEmboldenAmountUniform(objectIndex, uniforms);
}


void
StencilAAAStrategy::clearForResolve(Renderer& renderer)
{
  GLDEBUG(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
  GLDEBUG(glClear(GL_COLOR_BUFFER_BIT));
}

void
StencilAAAStrategy::createVAO(Renderer& renderer)
{
  if (!renderer.getMeshesAttached()) {
    return;
  }

  RenderContext& renderContext = *renderer.getRenderContext();
  PathfinderShaderProgram& program = *renderContext.getShaderManager().getProgram(program_stencilAAA);
  std::map<std::string, GLint>&  attributes = program.getAttributes();

  // was vertexArrayObjectExt.createVertexArrayOES
  GLDEBUG(glCreateVertexArrays(1, &mVAO));
  // was vertexArrayObjectExt.bindVertexArrayOES
  GLDEBUG(glBindVertexArray(mVAO));

  GLuint vertexPositionsBuffer = renderer.getMeshBuffers()[0]->stencilSegments;
  GLuint vertexNormalsBuffer = renderer.getMeshBuffers()[0]->stencilNormals;
  GLuint pathIDsBuffer = renderer.getMeshBuffers()[0]->stencilSegmentPathIDs;

  GLDEBUG(glUseProgram(program.getProgram()));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, renderContext.quadPositionsBuffer()));
  GLDEBUG(glVertexAttribPointer(attributes["aTessCoord"], 2, GL_FLOAT, GL_FALSE, 0, 0));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, vertexPositionsBuffer));
  GLDEBUG(glVertexAttribPointer(attributes["aFromPosition"], 2, GL_FLOAT, GL_FALSE, sizeof(float) * 6, 0));
  GLDEBUG(glVertexAttribPointer(attributes["aCtrlPosition"],
    2,
    GL_FLOAT,
    GL_FALSE,
    FLOAT32_SIZE * 6,
    (void*)(FLOAT32_SIZE * 2)));
  GLDEBUG(glVertexAttribPointer(attributes["aToPosition"],
    2,
    GL_FLOAT,
    false,
    FLOAT32_SIZE * 6,
    (void*)(FLOAT32_SIZE * 4)));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, vertexNormalsBuffer));
  GLDEBUG(glVertexAttribPointer(attributes["aFromNormal"], 2, GL_FLOAT, GL_FALSE, FLOAT32_SIZE * 6, 0));
  GLDEBUG(glVertexAttribPointer(attributes["aCtrlNormal"],
    2,
    GL_FLOAT,
    false,
    FLOAT32_SIZE * 6,
    (void*)(FLOAT32_SIZE * 2)));
  GLDEBUG(glVertexAttribPointer(attributes["aToNormal"],
    2,
    GL_FLOAT,
    false,
    FLOAT32_SIZE * 6,
    (void*)(FLOAT32_SIZE * 4));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, pathIDsBuffer)));
  GLDEBUG(glVertexAttribPointer(attributes["aPathID"], 1, GL_UNSIGNED_SHORT, GL_FALSE, 0, 0));

  GLDEBUG(glEnableVertexAttribArray(attributes["aTessCoord"]));
  GLDEBUG(glEnableVertexAttribArray(attributes["aFromPosition"]));
  GLDEBUG(glEnableVertexAttribArray(attributes["aCtrlPosition"]));
  GLDEBUG(glEnableVertexAttribArray(attributes["aToPosition"]));
  GLDEBUG(glEnableVertexAttribArray(attributes["aFromNormal"]));
  GLDEBUG(glEnableVertexAttribArray(attributes["aCtrlNormal"]));
  GLDEBUG(glEnableVertexAttribArray(attributes["aToNormal"]));
  GLDEBUG(glEnableVertexAttribArray(attributes["aPathID"]));

  // was instancedArraysExt.vertexAttribDivisorANGLE
  GLDEBUG(glVertexAttribDivisor(attributes["aFromPosition"], 1));
  GLDEBUG(glVertexAttribDivisor(attributes["aCtrlPosition"], 1));
  GLDEBUG(glVertexAttribDivisor(attributes["aToPosition"], 1));
  GLDEBUG(glVertexAttribDivisor(attributes["aFromNormal"], 1));
  GLDEBUG(glVertexAttribDivisor(attributes["aCtrlNormal"], 1));
  GLDEBUG(glVertexAttribDivisor(attributes["aToNormal"], 1));
  GLDEBUG(glVertexAttribDivisor(attributes["aPathID"], 1));

  GLDEBUG(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderContext.quadElementsBuffer()));

  // was vertexArrayObjectExt.bindVertexArrayOES
  GLDEBUG(glBindVertexArray(0));
}

void
StencilAAAStrategy::setBlendModeForAA(Renderer& renderer)
{
  GLDEBUG(glBlendEquation(GL_FUNC_ADD));
  GLDEBUG(glBlendFunc(GL_ONE, GL_ONE));
  GLDEBUG(glEnable(GL_BLEND));
}

AdaptiveStencilMeshAAAStrategy::AdaptiveStencilMeshAAAStrategy(int level, SubpixelAAType subpixelAA)
  : AntialiasingStrategy(subpixelAA)
{
  mMeshStrategy = std::unique_ptr<MCAAStrategy>(new MCAAStrategy(level, subpixelAA));
  mStencilStrategy = std::unique_ptr<StencilAAAStrategy>(new StencilAAAStrategy(level, subpixelAA));
}

DirectRenderingMode
AdaptiveStencilMeshAAAStrategy::getDirectRenderingMode() const
{
  return drm_none;
}

int
AdaptiveStencilMeshAAAStrategy::getPassCount() const
{
  return 1;
}

void
AdaptiveStencilMeshAAAStrategy::init(Renderer& renderer)
{
  mMeshStrategy->init(renderer);
  mStencilStrategy->init(renderer);
}

void
AdaptiveStencilMeshAAAStrategy::attachMeshes(RenderContext& renderContext, Renderer& renderer)
{
  mMeshStrategy->attachMeshes(renderContext, renderer);
  mStencilStrategy->attachMeshes(renderContext, renderer);
}

void
AdaptiveStencilMeshAAAStrategy::setFramebufferSize(Renderer& renderer)
{
  mMeshStrategy->setFramebufferSize(renderer);
  mStencilStrategy->setFramebufferSize(renderer);
}

kraken::Matrix4
AdaptiveStencilMeshAAAStrategy::getTransform() const
{
  return mMeshStrategy->getTransform();
}

void
AdaptiveStencilMeshAAAStrategy::prepareForRendering(Renderer& renderer)
{
  getAppropriateStrategy(renderer).prepareForRendering(renderer);
}

void
AdaptiveStencilMeshAAAStrategy::prepareForDirectRendering(Renderer& renderer)
{
  getAppropriateStrategy(renderer).prepareForDirectRendering(renderer);
}

void
AdaptiveStencilMeshAAAStrategy::finishAntialiasingObject(Renderer& renderer, int objectIndex)
{
  getAppropriateStrategy(renderer).finishAntialiasingObject(renderer, objectIndex);
}

void
AdaptiveStencilMeshAAAStrategy::prepareToRenderObject(Renderer& renderer, int objectIndex)
{
  getAppropriateStrategy(renderer).prepareToRenderObject(renderer, objectIndex);
}

void
AdaptiveStencilMeshAAAStrategy::finishDirectlyRenderingObject(Renderer& renderer, int objectIndex)
{
  getAppropriateStrategy(renderer).finishDirectlyRenderingObject(renderer, objectIndex);
}

void
AdaptiveStencilMeshAAAStrategy::antialiasObject(Renderer& renderer, int objectIndex)
{
  getAppropriateStrategy(renderer).antialiasObject(renderer, objectIndex);
}

void
AdaptiveStencilMeshAAAStrategy::resolveAAForObject(Renderer& renderer, int objectIndex)
{
  getAppropriateStrategy(renderer).resolveAAForObject(renderer, objectIndex);
}

void
AdaptiveStencilMeshAAAStrategy::resolve(int pass, Renderer& renderer)
{
  getAppropriateStrategy(renderer).resolve(pass, renderer);
}

kraken::Matrix4
AdaptiveStencilMeshAAAStrategy::getWorldTransformForPass(Renderer& renderer, int pass)
{
  return Matrix4::Identity();
}

AntialiasingStrategy& AdaptiveStencilMeshAAAStrategy::getAppropriateStrategy(Renderer& renderer)
{
  if (renderer.getNeedsStencil()) {
    return *mStencilStrategy;
  } else {
    return *mMeshStrategy;
  }
}

int
calculateStartFromIndexRanges(Range pathRange, std::vector<Range>& indexRanges)
{
return indexRanges.size() == 0 ? 0 : indexRanges[pathRange.start - 1].start;
}

int
calculateCountFromIndexRanges(Range pathRange, std::vector<Range>& indexRanges)
{
  if (indexRanges.size() == 0) {
    return 0;
  }

  int lastIndex;
  if (pathRange.end - 1 >= indexRanges.size()) {
    lastIndex = indexRanges.back().end;
  } else {
    lastIndex = indexRanges[pathRange.end - 1].start;
  }

  int firstIndex = indexRanges[pathRange.start - 1].start;

  return lastIndex - firstIndex;
}

} // namespace pathfinder
