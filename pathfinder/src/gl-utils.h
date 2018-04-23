// pathfinder/src/gl-utils.h
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#ifndef PATHFINDER_GL_UTILS_H
#define PATHFINDER_GL_UTILS_H

#include "platform.h"
#include <hydra.h>
#include <GLFW/glfw3.h>
#include <map>

#if defined(DEBUG) || defined(_DEBUG)
#define GLDEBUG(x) \
{ \
GLenum e; \
while( (e=glGetError()) != GL_NO_ERROR) \
{ \
fprintf(stderr, "Error before line number %d, in file %s. glGetError() returned %i\n" ,__LINE__, __FILE__, e ); \
} \
} \
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

const float QUAD_POSITIONS[] = {
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f
};
const size_t QUAD_POSITIONS_LENGTH = 8;

const float QUAD_TEX_COORDS[] ={
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f,
};
const size_t QUAD_TEX_COORDS_LENGTH = 8;

const __uint8_t QUAD_ELEMENTS[] = {
  0, 1, 2, 1, 3, 2
};
const size_t QUAD_ELEMENTS_LENGTH = 6;

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
