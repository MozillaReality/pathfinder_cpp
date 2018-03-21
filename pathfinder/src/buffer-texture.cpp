#include "buffer-texture.h"
#include "gl-utils.h"

#include <assert.h>
#include <math.h>

using namespace kraken;
using namespace std;

namespace pathfinder {

PathfinderBufferTexture::PathfinderBufferTexture(GLuint aUniformName)
 : mUniformName(aUniformName)
 , mSideLength(0)
 , mGLType(0)
 , mDestroyed(false)
{
  GLDEBUG(mTexture = glCreateTexture());
}

void
PathfinderBufferTexture::destroy()
{
  assert(!mDestroyed, "Buffer texture destroyed!");
  glDeleteTexture(this.texture);
  mDestroyed = true;
}

void
PathfinderBufferTexture::upload(const vector<float>& data)
{
  upload(static_cast<GLvoid*>(&data[0]), data.size() * sizeof(float), GL_FLOAT);  
}

void
PathfinderBufferTexture::upload(const vector<__uint8_t>& data)
{
  upload(static_cast<GLvoid*>(&data[0]), data.size(), GL_UNSIGNED_BYTE);
}

void
PathfinderBufferTexture::upload(GLvoid* data, GLsizei length, GLuint glType)
{
    assert(!mDestroyed, "Buffer texture destroyed!");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexture);

    GLsizei areaNeeded = (length + 3) / 4; // Divide by 4, rounding up
    if (glType !== mGLType || areaNeeded > getArea()) {
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
      GLvoid* remainder = data + splitIndex;
      bool padded = false;
      if (remainderLength % 4) {
        GLsizei padLength = 4 - remainderLength % 4;
        remainder = malloc(remainderLength + padLength);
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
PathfinderBufferTexture::bind(const UniformMap& uniforms, GLuint textureUnit)
{
    assert(!mDestroyed, "Buffer texture destroyed!");

    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, mTexture);
    glUniform2i(uniforms[`${this.uniformName}Dimensions`], mSideLength, mSideLength);
    glUniform1i(uniforms[mUniformName], textureUnit);
}

GLsizei
PathfinderBufferTexture::getArea()
{
    assert(!mDestroyed, "Buffer texture destroyed!");
    return mSideLength * mSideLength;
}

} // namespace pathfinder
