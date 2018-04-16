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
  mDestFramebufferSize = Vector2i::Zero();
  mSupersampledFramebufferSize = Vector2i::Zero();
}

SSAAStrategy::~SSAAStrategy()
{
  if (supersampledFramebuffer) {
    GLDEBUG(glDeleteFramebuffers(1, &supersampledFramebuffer));
    supersampledFramebuffer = 0;
  }
  if (supersampledColorTexture) {
    GLDEBUG(glDeleteTextures(1, &supersampledColorTexture));
    supersampledColorTexture = 0;
  }
  if (supersampledDepthTexture) {
    GLDEBUG(glDeleteTextures(1, &supersampledDepthTexture));
    supersampledDepthTexture = 0;
  }
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

bool
SSAAStrategy::init(Renderer& renderer)
{
  if (!AntialiasingStrategy::init(renderer)) {
    return false;
  }
  mDestFramebufferSize = renderer.getDestAllocatedSize();

  mSupersampledFramebufferSize = Vector2i::Create(
    mDestFramebufferSize.x * supersampleScale().x,
    mDestFramebufferSize.y * supersampleScale().y);

  supersampledColorTexture =
    createFramebufferColorTexture(
      mSupersampledFramebufferSize.x,
      mSupersampledFramebufferSize.y,
      renderer.getRenderContext()->getColorAlphaFormat(),
      GL_LINEAR);

  supersampledDepthTexture =
    createFramebufferDepthTexture(mSupersampledFramebufferSize);

  supersampledFramebuffer = createFramebuffer(supersampledColorTexture, supersampledDepthTexture);

  GLDEBUG(glBindFramebuffer(GL_FRAMEBUFFER, 0));
  return true;
}

Matrix4
SSAAStrategy::getTransform() const
{
  Vector3 scale =  Vector3::Create(
    (float)mSupersampledFramebufferSize.x / (float)mDestFramebufferSize.x,
    (float)mSupersampledFramebufferSize.y / (float)mDestFramebufferSize.y,
    1.0f);

  return Matrix4::Scaling(scale);
}

void
SSAAStrategy::prepareForRendering(Renderer& renderer)
{
  Vector2i usedSize = usedSupersampledFramebufferSize(renderer);
  GLDEBUG(glBindFramebuffer(GL_FRAMEBUFFER, supersampledFramebuffer));
  GLDEBUG(glViewport(0, 0, mSupersampledFramebufferSize[0], mSupersampledFramebufferSize[1]));
  GLDEBUG(glScissor(0, 0, usedSize[0], usedSize[1]));
  GLDEBUG(glEnable(GL_SCISSOR_TEST));

  Vector4 clearColor = renderer.getBGColor();
  GLDEBUG(glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]));
  GLDEBUG(glClearDepth(0.0));
  GLDEBUG(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void
SSAAStrategy::prepareToRenderObject(Renderer& renderer, int objectIndex)
{
  GLDEBUG(glBindFramebuffer(GL_FRAMEBUFFER, supersampledFramebuffer));
  GLDEBUG(glViewport(0,
    0,
    mSupersampledFramebufferSize[0],
    mSupersampledFramebufferSize[1]));
  GLDEBUG(glDisable(GL_SCISSOR_TEST));
}

void
SSAAStrategy::resolve(int pass, Renderer& renderer)
{
  RenderContext& renderContext = *renderer.getRenderContext();
  GLDEBUG(glBindFramebuffer(GL_FRAMEBUFFER, renderer.getDestFramebuffer()));
  GLDEBUG(glViewport(0, 0, renderer.getDestAllocatedSize()[0], renderer.getDestAllocatedSize()[1]));
  GLDEBUG(glDisable(GL_DEPTH_TEST));
  GLDEBUG(glDisable(GL_BLEND));

  // Set up the blit program VAO.
  PathfinderShaderProgram& resolveProgram = *renderContext.getShaderManager().getProgram(mSubpixelAA == saat_none ? program_blitLinear : program_ssaaSubpixelResolve);

  GLDEBUG(glUseProgram(resolveProgram.getProgram()));
  renderContext.initQuadVAO(resolveProgram);

  // Resolve framebuffer.
  GLDEBUG(glActiveTexture(GL_TEXTURE0));
  GLDEBUG(glBindTexture(GL_TEXTURE_2D, supersampledColorTexture));
  GLDEBUG(glUniform1i(resolveProgram.getUniform(uniform_uSource), 0));
  if (resolveProgram.hasUniform(uniform_uSourceDimensions)) {
    GLDEBUG(glUniform2i(resolveProgram.getUniform(uniform_uSourceDimensions),
      mSupersampledFramebufferSize[0],
      mSupersampledFramebufferSize[1]));
  }

  TileInfo tileInfo = tileInfoForPass(pass);
  renderer.setTransformAndTexScaleUniformsForDest(resolveProgram, &tileInfo);
  GLDEBUG(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderContext.quadElementsBuffer()));
  GLDEBUG(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0));
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
