// pathfinder/src/aa-strategy.cpp
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include "aa-strategy.h"
#include "renderer.h"

#include "platform.h"

namespace pathfinder {

bool
AntialiasingStrategy::init(Renderer& renderer)
{
  setFramebufferSize(renderer);
  return true;
}

kraken::Matrix4
AntialiasingStrategy::getWorldTransformForPass(Renderer& renderer, int pass)
{
  return kraken::Matrix4::Identity();
}

void
AntialiasingStrategy::setSubpixelAAKernelUniform(Renderer& renderer, PathfinderShaderProgram& aProgram)
{
  if (aProgram.hasUniform(uniform_uKernel)) {
    const float* kernel = SUBPIXEL_AA_KERNELS[mSubpixelAA];
    GLDEBUG(glUniform4f(aProgram.getUniform(uniform_uKernel), kernel[0], kernel[1], kernel[2], kernel[3]));
  }
}

void
NoAAStrategy::setFramebufferSize(Renderer& renderer)
{
  mFramebufferSize = renderer.getDestAllocatedSize();
};

void
NoAAStrategy::prepareForRendering(Renderer& renderer)
{
  GLDEBUG(glBindFramebuffer(GL_FRAMEBUFFER, renderer.getDestFramebuffer()));
  GLDEBUG(glViewport(0, 0, mFramebufferSize[0], mFramebufferSize[1]));
  GLDEBUG(glDisable(GL_SCISSOR_TEST));
}

void
NoAAStrategy::prepareToRenderObject(Renderer& renderer, int objectIndex)
{
  GLDEBUG(glBindFramebuffer(GL_FRAMEBUFFER, renderer.getDestFramebuffer()));
  GLDEBUG(glViewport(0, 0, mFramebufferSize[0], mFramebufferSize[1]));
  GLDEBUG(glDisable(GL_SCISSOR_TEST));
}

} // namespace pathfinder
