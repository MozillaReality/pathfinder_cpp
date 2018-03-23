#ifndef PATHFINDER_BUFFER_TEXTURE_H
#define PATHFINDER_BUFFER_TEXTURE_H

#include "gl-utils.h"

#include "platform.h"
#include <kraken-math.h>
#include <vector>
#include <memory>

namespace pathfinder {

class PathfinderBufferTexture
{
public:
  PathfinderBufferTexture(GLuint aUniformName);
  void destroy();

  GLuint getTexture() const;
  GLuint getUniformName() const;
  void upload(const std::vector<float>& data);
  void upload(const std::vector<__uint8_t>& data);
  void bind(const UniformMap& uniforms, GLuint textureUnit);

private:
  GLuint mTexture;
  GLuint mUniformName;
  GLsizei mSideLength;
  GLuint mGLType;
  bool mDestroyed;

  void upload(GLvoid* data, GLsizei length, GLuint glType);
  GLsizei getArea();
};

} // namespace pathfinder

#endif // PATHFINDER_BUFFER_TEXTURE_H
