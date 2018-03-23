#include "gl-utils.h"

#include "platform.h"

namespace pathfinder {

void
setTextureParameters(GLint aFilter)
{
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, aFilter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, aFilter);
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
