#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

#include <pathfinder.h>
#include "renderer.h"

const char* shader_blit_vs =
#include "resources/shaders/blit.vs.glsl"
;

const char* shader_blit_linear_fs =
#include "resources/shaders/blit-linear.fs.glsl"
;

void pathfinder_init()
{
  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &shader_blit_vs, NULL);
  glCompileShader(vs);
  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &shader_blit_linear_fs, NULL);
  glCompileShader(fs);

  GLuint shader_programme = glCreateProgram();
  glAttachShader(shader_programme, fs);
  glAttachShader(shader_programme, vs);
  glLinkProgram(shader_programme);
}
