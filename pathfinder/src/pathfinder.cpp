#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

#include <pathfinder.h>
#include "renderer.h"


#define DEBUG
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

const char* shader_blit_vs =
#include "resources/shaders/blit.vs.glsl"
;

const char* shader_blit_linear_fs =
#include "resources/shaders/blit-linear.fs.glsl"
;

void pathfinder_init()
{
  GLDEBUG(GLuint vs = glCreateShader(GL_VERTEX_SHADER));
  GLDEBUG(glShaderSource(vs, 1, &shader_blit_vs, NULL));
  GLDEBUG(glCompileShader(vs));
  GLDEBUG(GLuint fs = glCreateShader(GL_FRAGMENT_SHADER));
  GLDEBUG(glShaderSource(fs, 1, &shader_blit_linear_fs, NULL));
  GLDEBUG(glCompileShader(fs));
  GLDEBUG(GLuint shader_program = glCreateProgram());
  GLDEBUG(glAttachShader(shader_program, fs));
  GLDEBUG(glAttachShader(shader_program, vs));
  GLDEBUG(glLinkProgram(shader_program));
}

