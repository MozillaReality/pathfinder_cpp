#include "xcaa-strategy.h"

#include "renderer.h"
#include "gl-utils.h"
#include <kraken-math.h>
#include <memory>

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
  glScissor(0, 0, usedSize[0], usedSize[1]);
  glEnable(GL_SCISSOR_TEST);

  // Clear out the color and depth textures.
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glClearDepth(0.0);
  glDepthMask(GL_TRUE);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
  glScissor(0, 0, usedSize[0], usedSize[1]);
  glEnable(GL_SCISSOR_TEST);
  setDepthAndBlendModeForResolve();

  // Clear out the resolve buffer, if necessary.
  clearForResolve(renderer);

  // Resolve.
  glUseProgram(resolveProgram.getProgram());
  // was renderContext.vertexArrayObjectExt
  glBindVertexArray(resolveVAO);
  glUniform2i(resolveProgram.getUniforms()["uFramebufferSize"],
    destFramebufferSize[0],
    destFramebufferSize[1]);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, aaAlphaTexture);
  glUniform1i(resolveProgram.getUniforms()["uAAAlpha"], 0);
  glUniform2i(resolveProgram.getUniforms()["uAAAlphaDimensions"],
    supersampledFramebufferSize[0],
    supersampledFramebufferSize[1]);
  if (renderer.getBGColor() != Vector4::Zero()) {
    glUniform4fv(resolveProgram.getUniforms()["uBGColor"], sizeof(float) * 4, renderer.getBGColor().c);
  }
  if (renderer.getFGColor() != Vector4::Zero()) {
    glUniform4fv(resolveProgram.getUniforms()["uFGColor"], sizeof(float) * 4, renderer.getFGColor().c);
  }
  renderer.setTransformSTAndTexScaleUniformsForDest(resolveProgram.getUniforms());
  setSubpixelAAKernelUniform(renderer, resolveProgram.getUniforms());
  setAdditionalStateForResolveIfNecessary(renderer, resolveProgram, 1);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);
  // was vertexArrayObjectExt.bindVertexArrayOES
  glBindVertexArray(0);
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
    glBindFramebuffer(GL_FRAMEBUFFER, aaFramebuffer);
  }
  glViewport(0,
             0,
             supersampledFramebufferSize[0],
             supersampledFramebufferSize[1]);
  glScissor(0, 0, usedSize[0], usedSize[1]);
  glEnable(GL_SCISSOR_TEST);
}

void
XCAAStrategy::setAAState(Renderer& renderer)
{
  Vector2i usedSize = supersampledUsedSize(renderer);
  if (usesAAFramebuffer(renderer))
    glBindFramebuffer(GL_FRAMEBUFFER, aaFramebuffer);
  glViewport(0,
             0,
             supersampledFramebufferSize[0],
             supersampledFramebufferSize[1]);
  glScissor(0, 0, usedSize[0], usedSize[1]);
  glEnable(GL_SCISSOR_TEST);

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

  glUniform2i(uniforms["uFramebufferSize"],
              supersampledFramebufferSize[0],
              supersampledFramebufferSize[1]);
  renderer.getPathTransformBufferTextures()[0]->ext->bind(uniforms, 0);
  renderer.getPathTransformBufferTextures()[0]->st->bind(uniforms, 1);
  pathBoundsBufferTextures[objectIndex]->bind(uniforms, 2);
  renderer.setHintsUniform(uniforms);
  renderer.bindAreaLUT(4, uniforms);
}

void
XCAAStrategy::setDepthAndBlendModeForResolve()
{
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
}

void
XCAAStrategy::initResolveFramebufferForObject(Renderer& renderer, int objectIndex) {
  glBindFramebuffer(GL_FRAMEBUFFER, renderer.getDestFramebuffer());
  glViewport(0, 0, destFramebufferSize[0], destFramebufferSize[1]);
  glDisable(GL_SCISSOR_TEST);
}

void
XCAAStrategy::initAAAlphaFramebuffer(Renderer& renderer)
{
  if (!getMightUseAAFramebuffer()) {
    glDeleteTextures(1, &aaAlphaTexture);
    aaAlphaTexture = 0;
    glDeleteTextures(1, &aaDepthTexture);
    aaDepthTexture = 0;
    glDeleteTextures(1, &aaFramebuffer);
    aaFramebuffer = 0;
    return;
  }
  glCreateTextures(GL_TEXTURE_2D, 1, &aaAlphaTexture);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, aaAlphaTexture);
  glTexImage2D(GL_TEXTURE_2D,
    0,
    GL_RGB,
    supersampledFramebufferSize[0],
    supersampledFramebufferSize[1],
    0,
    GL_RGB,
    GL_HALF_FLOAT, // was renderContext.textureHalfFloatExt.GL_HALF_FLOAT_OES
    0);
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
      move(make_unique<PathfinderBufferTexture>("uPathBounds"));
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
  glCreateVertexArrays(1, &resolveVAO);
  // was vertexArrayObjectExt.bindVertexArrayOES
  glBindVertexArray(resolveVAO);

  glUseProgram(resolveProgram.getProgram());
  renderContext.initQuadVAO(resolveProgram.getAttributes());

  // was vertexArrayObjectExt.bindVertexArrayOES
  glBindVertexArray(0);
}

} // namespace pathfinder
