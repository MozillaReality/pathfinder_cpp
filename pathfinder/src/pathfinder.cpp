#include "platform.h"
#include <GLFW/glfw3.h>
#include <stdio.h>

#include <pathfinder.h>
#include "renderer.h"

#include "gl-utils.h"

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

