// pathfinder/src/gl-utils.cpp
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include "gl-utils.h"

#include "platform.h"
#include <assert.h>

namespace pathfinder {

GLuint createFramebufferDepthTexture(kraken::Vector2i size)
{
  GLuint texture = 0;
  GLDEBUG(glCreateTextures(GL_TEXTURE_2D, 1, &texture));
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_DEPTH_COMPONENT,
               size[0],
               size[1],
               0,
               GL_DEPTH_COMPONENT,
               GL_UNSIGNED_INT,
               0);
  setTextureParameters(GL_NEAREST);
  return texture;
}

void
setTextureParameters(GLint aFilter)
{
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, aFilter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, aFilter);
}

GLuint
createFramebuffer(GLuint colorAttachment, GLuint depthAttachment)
{
  GLuint framebuffer = 0;
  glCreateFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

  glFramebufferTexture2D(GL_FRAMEBUFFER,
                         GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D,
                         colorAttachment,
                         0);

  if (depthAttachment != 0) {
    glFramebufferTexture2D(GL_FRAMEBUFFER,
    GL_DEPTH_ATTACHMENT,
    GL_TEXTURE_2D,
    depthAttachment,
    0);
    GLint param;
    glGetFramebufferAttachmentParameteriv(
      GL_FRAMEBUFFER,
      GL_DEPTH_ATTACHMENT,
      GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
      &param);
    assert(param == GL_TEXTURE);
  }

  assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  return framebuffer;
}



GLuint
createFramebufferColorTexture(GLsizei width,
                              GLsizei height,
                              ColorAlphaFormat colorAlphaFormat,
                              GLint filter)
{
  GLuint format;
  GLuint internalFormat;
  GLuint type;
  size_t bufferSize;
  GLvoid* zeroes = nullptr;
  if (colorAlphaFormat == caf_RGBA8) {
      format = internalFormat = GL_RGBA;
      type = GL_UNSIGNED_BYTE;
      bufferSize = width * height * 4;
  } else {
      format = internalFormat = GL_RGBA;
      type = GL_UNSIGNED_SHORT_5_5_5_1;
      bufferSize = width * height * 2;
  }
  zeroes = malloc(bufferSize);
  memset(zeroes, 0, bufferSize);

  GLuint texture = 0;
  GLDEBUG(glCreateTextures(GL_TEXTURE_2D, 1, &texture));
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, zeroes);
  setTextureParameters(filter);

  free(zeroes);
  return texture;
}

} // namespace pathfinder
