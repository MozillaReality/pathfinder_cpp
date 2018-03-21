#ifndef PATHFINDER_GL_UTILS_H
#define PATHFINDER_GL_UTILS_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <map>

#if defined(DEBUG) || defined(_DEBUG)
#define GLDEBUG(x) \
x; \
{ \
GLenum e; \
while( (e=glGetError()) != GL_NO_ERROR) \
{ \
fprintf(stderr, "Error at line number %d, in file %s. glGetError() returned %i for call %s\n",__LINE__, __FILE__, e, #x ); \
} \
}
#else
#define GLDEBUG(x) x;
#endif

namespace pathfinder {

typedef enum {
 caf_RGBA8,
 caf_RGB5_A1
} ColorAlphaFormat;

void setTextureParameters(GLint aFilter);
GLuint createFramebufferColorTexture(GLsizei width,
                                     GLsizei height,
                                     ColorAlphaFormat colorAlphaFormat,
                                     GLint filter = GL_NEAREST);

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
