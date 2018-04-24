// pathfinder/src/buffer-texture.cpp
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include "buffer-texture.h"
#include "gl-utils.h"
#include "platform.h"
#include "shader-loader.h"

#include <assert.h>
#include <math.h>

using namespace kraken;
using namespace std;

namespace pathfinder {

PathfinderBufferTexture::PathfinderBufferTexture(UniformID aUniformID, UniformID aUniformDimensionsID)
  : mTexture(0)
  , mUniformID(aUniformID)
  , mUniformDimensionsID(aUniformDimensionsID)
  , mSideLength(0)
  , mGLType(0)
  , mDestroyed(false)
{
  GLDEBUG(glCreateTextures(GL_TEXTURE_2D, 1, &mTexture));
}

PathfinderBufferTexture::~PathfinderBufferTexture()
{
  if (!mDestroyed) {
    destroy();
  }
}

void
PathfinderBufferTexture::destroy()
{
  assert(!mDestroyed);
  GLDEBUG(glDeleteTextures(1, &mTexture));
  mTexture = 0;
  mDestroyed = true;
}

void
PathfinderBufferTexture::upload(const vector<float>& data)
{
  upload((__uint8_t*)(&data[0]), (GLsizei)data.size(), GL_FLOAT);
}

void
PathfinderBufferTexture::upload(const vector<__uint8_t>& data)
{
  upload((__uint8_t*)(&data[0]), (GLsizei)data.size(), GL_UNSIGNED_BYTE);
}

void
PathfinderBufferTexture::upload(__uint8_t* data, GLsizei length, GLuint glType)
{
  assert(!mDestroyed);

  GLDEBUG(glActiveTexture(GL_TEXTURE0));
  GLDEBUG(glBindTexture(GL_TEXTURE_2D, mTexture));

  GLsizei areaNeeded = (length + 3) / 4; // Divide by 4, rounding up
  if (glType != mGLType || areaNeeded > getArea()) {
      mSideLength = 1;
      while (mSideLength * mSideLength < areaNeeded) {
        mSideLength <<= 1;
      }
      mGLType = glType;
      setTextureParameters(GL_NEAREST);
      GLDEBUG(glTexImage2D(GL_TEXTURE_2D,
                           0,
                           glType == GL_FLOAT ? GL_RGBA32F : GL_RGBA,
                           mSideLength,
                           mSideLength,
                           0,
                           GL_RGBA,
                           glType,
                           NULL));
  }

  GLsizei mainDimensionsHeight = areaNeeded / mSideLength; 
  GLsizei remainderDimensionsWidth = areaNeeded % mSideLength;
  GLsizei splitIndex = mSideLength * mainDimensionsHeight * 4;

  if (mSideLength > 0 && mainDimensionsHeight > 0) {
    GLDEBUG(glTexSubImage2D(GL_TEXTURE_2D,
                            0,
                            0,
                            0,
                            mSideLength,
                            mainDimensionsHeight,
                            GL_RGBA,
                            glType,
                            data));
  }

  if (remainderDimensionsWidth > 0) {
    int typeSize = glType == GL_FLOAT ? 4 : 1;
    // Round data up to a multiple of 4 elements if necessary.
    GLsizei remainderLength = length - splitIndex;
    __uint8_t* remainder = data + splitIndex * typeSize;
    bool padded = false;
    if (remainderLength % 4) {
      GLsizei padLength = 4 - remainderLength % 4;
      remainder = (__uint8_t*)malloc((remainderLength + padLength) * typeSize);
      memcpy(remainder, data + splitIndex * typeSize, remainderLength * typeSize);
      for(int i=0; i<padLength * typeSize; i++) {
        remainder[remainderLength * typeSize + i] = 0;
      }
      padded = true;
    }

    GLDEBUG(glTexSubImage2D(GL_TEXTURE_2D,
                    0,
                    0,
                    mainDimensionsHeight,
                    remainderDimensionsWidth,
                    1,
                    GL_RGBA,
                    glType,
                    remainder));
    if (padded) {
      free(remainder);
    }
  }
}

void
PathfinderBufferTexture::bind(PathfinderShaderProgram& aProgram, GLuint textureUnit)
{
  assert(!mDestroyed);

  GLDEBUG(glActiveTexture(GL_TEXTURE0 + textureUnit));
  GLDEBUG(glBindTexture(GL_TEXTURE_2D, mTexture));
  if (aProgram.hasUniform(mUniformDimensionsID)) {
    GLDEBUG(glUniform2i(aProgram.getUniform(mUniformDimensionsID), mSideLength, mSideLength));
  }
  if (aProgram.hasUniform(mUniformID)) {
    GLDEBUG(glUniform1i(aProgram.getUniform(mUniformID), textureUnit));
  }
}

GLsizei
PathfinderBufferTexture::getArea()
{
  assert(!mDestroyed);
  return mSideLength * mSideLength;
}

} // namespace pathfinder
