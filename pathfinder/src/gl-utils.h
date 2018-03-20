#ifndef PATHFINDER_GL_UTILS_H
#define PATHFINDER_GL_UTILS_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <map>

namespace pathfinder {

void setTextureParameters(GLint aFilter);
struct UniformMap
{
public:
  UniformMap()
    : uFramebufferSize(0)
    , uTransform(0)
    , uTexScale(0)
    , uTransformST(0)
    , uTransformExt(0)
    , uEmboldenAmount(0)
  { };

  GLuint uFramebufferSize;
  GLuint uTransform;
  GLuint uTexScale;
  GLuint uTransformST;
  GLuint uTransformExt;
  GLuint uEmboldenAmount;
};

} // namespace pathfinder

#endif // PATHFINDER_GL_UTILS_H
