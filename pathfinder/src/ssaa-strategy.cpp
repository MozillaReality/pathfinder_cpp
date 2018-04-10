// pathfinder/src/ssaa-strategy.cpp
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
#include "ssaa-strategy.h"

#include "renderer.h"

#include <kraken-math.h>

using namespace std;
using namespace kraken;

namespace pathfinder {

SSAAStrategy::SSAAStrategy(int aLevel, SubpixelAAType aSubpixelAA)
  : AntialiasingStrategy(aSubpixelAA)
  , mLevel(aLevel)
  , supersampledColorTexture(0)
  , supersampledDepthTexture(0)
  , supersampledFramebuffer(0)
{
  destFramebufferSize = Vector2i::Zero();
  supersampledFramebufferSize = Vector2i::Zero();
}

int
SSAAStrategy::getPassCount() const
{
  switch (mLevel) {
  case 16:
    return 4;
  case 8:
    return 2;
  }
  return 1;
}

void
SSAAStrategy::setFramebufferSize(Renderer& renderer)
{
  destFramebufferSize = renderer.getDestAllocatedSize();

  supersampledFramebufferSize = Vector2i::Create(
    destFramebufferSize.x * supersampleScale().x,
    destFramebufferSize.y * supersampleScale().y);


  supersampledColorTexture =
    createFramebufferColorTexture(
      supersampledFramebufferSize.x,
      supersampledFramebufferSize.y,
      renderer.getRenderContext()->getColorAlphaFormat(),
      GL_LINEAR);

  supersampledDepthTexture =
    createFramebufferDepthTexture(supersampledFramebufferSize);

  supersampledFramebuffer = createFramebuffer(supersampledColorTexture, supersampledDepthTexture);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Matrix4
SSAAStrategy::getTransform() const
{
  Vector3 scale =  Vector3::Create(
    (float)supersampledFramebufferSize.x / (float)destFramebufferSize.x,
    (float)supersampledFramebufferSize.y / (float)destFramebufferSize.y,
    1.0f);

  return Matrix4::Scaling(scale);
}

void
SSAAStrategy::prepareForRendering(Renderer& renderer)
{
  Vector2i framebufferSize = supersampledFramebufferSize;
  Vector2i usedSize = usedSupersampledFramebufferSize(renderer);
  glBindFramebuffer(GL_FRAMEBUFFER, supersampledFramebuffer);
  glViewport(0, 0, framebufferSize[0], framebufferSize[1]);
  glScissor(0, 0, usedSize[0], usedSize[1]);
  glEnable(GL_SCISSOR_TEST);

  Vector4 clearColor = renderer.getBackgroundColor();
  glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
  glClearDepth(0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void
SSAAStrategy::prepareToRenderObject(Renderer& renderer, int objectIndex)
{
  glBindFramebuffer(GL_FRAMEBUFFER, supersampledFramebuffer);
  glViewport(0,
    0,
    supersampledFramebufferSize[0],
    supersampledFramebufferSize[1]);
  glDisable(GL_SCISSOR_TEST);
}

void
SSAAStrategy::resolve(int pass, Renderer& renderer)
{
  RenderContext& renderContext = *renderer.getRenderContext();
  glBindFramebuffer(GL_FRAMEBUFFER, renderer.getDestFramebuffer());
  glViewport(0, 0, renderer.getDestAllocatedSize()[0], renderer.getDestAllocatedSize()[1]);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  // Set up the blit program VAO.
  PathfinderShaderProgram& resolveProgram = *renderContext.getShaderManager().getProgram(mSubpixelAA == saat_none ? program_blitLinear : program_ssaaSubpixelResolve);

  glUseProgram(resolveProgram.getProgram());
  renderContext.initQuadVAO(resolveProgram.getAttributes());

  // Resolve framebuffer.
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, supersampledColorTexture);
  glUniform1i(resolveProgram.getUniforms()["uSource"], 0);
  glUniform2i(resolveProgram.getUniforms()["uSourceDimensions"],
    supersampledFramebufferSize[0],
    supersampledFramebufferSize[1]);
  TileInfo tileInfo = tileInfoForPass(pass);
  renderer.setTransformAndTexScaleUniformsForDest(resolveProgram.getUniforms(), &tileInfo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderContext.quadElementsBuffer());
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);
}

Matrix4
SSAAStrategy::getWorldTransformForPass(Renderer& renderer, int pass)
{
  TileInfo tileInfo = tileInfoForPass(pass);
  Vector2i usedSize = renderer.getDestUsedSize();
  Matrix4 transform = Matrix4::Translation(Vector3::Create(-1.0f, -1.0f, 0.0f));
  transform.scale((float)tileInfo.size.x, (float)tileInfo.size.y, 1.0f);
  transform.translate(Vector3::Create(
    -tileInfo.position[0] / tileInfo.size[0] * 2.0f,
    -tileInfo.position[1] / tileInfo.size[1] * 2.0f,
    0.0f
  ));
  transform.translate(Vector3::Create(1.0f, 1.0f, 0.0f));
  return transform;
}

TileInfo
SSAAStrategy::tileInfoForPass(int pass)
{
  Vector2i tileSize = getTileSize();
  TileInfo tileInfo;
  tileInfo.position = Vector2i::Create(pass % tileSize[0], pass / tileSize[0]);
  tileInfo.size = tileSize;
  return tileInfo;
}

DirectRenderingMode
SSAAStrategy::getDirectRenderingMode() const
{
  return drm_color;
}

Vector2i
SSAAStrategy::supersampleScale() const
{
  return Vector2i::Create(mSubpixelAA != saat_none ? 3 : 2, mLevel == 2 ? 1 : 2);

}

Vector2i
SSAAStrategy::getTileSize() const
{
  switch (mLevel) {
  case 16:
    return Vector2i::Create(2, 2);
  case 8:
    return Vector2i::Create(2, 1);
  }
  return Vector2i::One();
}

Vector2i
SSAAStrategy::usedSupersampledFramebufferSize(Renderer& renderer) const
{
  return Vector2i::Create(
    renderer.getDestUsedSize().x * supersampleScale().x,
    renderer.getDestUsedSize().y * supersampleScale().y
  );
}

} // namespace pathfinder
