#include "aa-strategy.h"
#include "renderer.h"

#include "platform.h"

namespace pathfinder {

void
AntialiasingStrategy::init(Renderer& renderer)
{
  setFramebufferSize(renderer);
}

kraken::Matrix4
AntialiasingStrategy::getWorldTransformForPass(Renderer& renderer, int pass)
{
  return kraken::Matrix4::Identity();
}

void
AntialiasingStrategy::setSubpixelAAKernelUniform(Renderer& renderer, UniformMap& uniforms)
{
  const float* kernel = SUBPIXEL_AA_KERNELS[mSubpixelAA];
  glUniform4f(uniforms["uKernel"], kernel[0], kernel[1], kernel[2], kernel[3]);
}

void
NoAAStrategy::setFramebufferSize(Renderer& renderer)
{
  mFramebufferSize = renderer.getDestAllocatedSize();
};

void
NoAAStrategy::prepareForRendering(Renderer& renderer)
{
  glBindFramebuffer(GL_FRAMEBUFFER, renderer.getDestFramebuffer());
  glViewport(0, 0, mFramebufferSize[0], mFramebufferSize[1]);
  glDisable(GL_SCISSOR_TEST);
}

void
NoAAStrategy::prepareToRenderObject(Renderer& renderer, int objectIndex)
{
  glBindFramebuffer(GL_FRAMEBUFFER, renderer.getDestFramebuffer());
  glViewport(0, 0, mFramebufferSize[0], mFramebufferSize[1]);
  glDisable(GL_SCISSOR_TEST);
}

} // namespace pathfinder
