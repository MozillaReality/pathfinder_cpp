#include "buffer-texture.h"
#include "gl-utils.h"
#include "platform.h"

#include <assert.h>
#include <math.h>

using namespace kraken;
using namespace std;

namespace pathfinder {

PathfinderBufferTexture::PathfinderBufferTexture(const std::string& aUniformName)
 : mTexture(0)
 , mUniformName(aUniformName)
 , mSideLength(0)
 , mGLType(0)
 , mDestroyed(false)
{
  GLDEBUG(glCreateTextures(GL_TEXTURE_2D, 1, &mTexture));
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
  upload((__uint8_t*)(&data[0]), (GLsizei)data.size() * sizeof(float), GL_FLOAT);  
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

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, mTexture);

  GLsizei areaNeeded = (length + 3) / 4; // Divide by 4, rounding up
  if (glType != mGLType || areaNeeded > getArea()) {
      mSideLength = 1;
      while (mSideLength * mSideLength < areaNeeded) {
        mSideLength <<= 1;
      }
      mGLType = glType;

      glTexImage2D(GL_TEXTURE_2D,
                    0,
                    GL_RGBA,
                    mSideLength,
                    mSideLength,
                    0,
                    GL_RGBA,
                    glType,
                    NULL);
      setTextureParameters(GL_NEAREST);
  }

  GLsizei mainDimensionsHeight = areaNeeded / mSideLength; 
  GLsizei remainderDimensionsWidth = areaNeeded % mSideLength;
  GLsizei splitIndex = mSideLength * mainDimensionsHeight * 4;

  if (mSideLength > 0 && mainDimensionsHeight > 0) {
      glTexSubImage2D(GL_TEXTURE_2D,
                        0,
                        0,
                        0,
                        mSideLength,
                        mainDimensionsHeight,
                        GL_RGBA,
                        glType,
                        data);
  }

  if (remainderDimensionsWidth > 0) {
    // Round data up to a multiple of 4 elements if necessary.
    GLsizei remainderLength = length - splitIndex;
    __uint8_t* remainder = data + splitIndex;
    bool padded = false;
    if (remainderLength % 4) {
      GLsizei padLength = 4 - remainderLength % 4;
      remainder = (__uint8_t*)malloc(remainderLength + padLength);
      memcpy(remainder, data + splitIndex, length - splitIndex);
      for(int i=0; i<padLength; i++) {
        remainder[length - splitIndex + i] = 0;
      }
      padded = true;
    }

    glTexSubImage2D(GL_TEXTURE_2D,
                    0,
                    0,
                    mainDimensionsHeight,
                    remainderDimensionsWidth,
                    1,
                    GL_RGBA,
                    glType,
                    remainder);

    if (padded) {
      free(remainder);
    }
  }
}

void
PathfinderBufferTexture::bind(UniformMap& uniforms, GLuint textureUnit)
{
  assert(!mDestroyed);

  glActiveTexture(GL_TEXTURE0 + textureUnit);
  glBindTexture(GL_TEXTURE_2D, mTexture);
  glUniform2i(uniforms[mUniformName + "Dimensions"], mSideLength, mSideLength);
  glUniform1i(uniforms[mUniformName], textureUnit);
}

GLsizei
PathfinderBufferTexture::getArea()
{
  assert(!mDestroyed);
  return mSideLength * mSideLength;
}

} // namespace pathfinder
