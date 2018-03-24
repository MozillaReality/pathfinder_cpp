#ifndef PATHFINDER_GL_UTILS_H
#define PATHFINDER_GL_UTILS_H

#include "platform.h"
#include <kraken-math.h>
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


GLuint createFramebufferDepthTexture(kraken::Vector2i size);
void setTextureParameters(GLint aFilter);
GLuint createFramebuffer(GLuint colorAttachment, GLuint depthAttachment);

GLuint createFramebufferColorTexture(GLsizei width,
                                     GLsizei height,
                                     ColorAlphaFormat colorAlphaFormat,
                                     GLint filter = GL_NEAREST);

typedef std::map<std::string, GLint> UniformMap;

} // namespace pathfinder

#endif // PATHFINDER_GL_UTILS_H
