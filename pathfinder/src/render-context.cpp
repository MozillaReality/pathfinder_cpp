// pathfinder/src/render-context.cpp
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include "render-context.h"
#include "gl-utils.h"

#include <string>

namespace pathfinder {


RenderContext::RenderContext()
  : mQuadPositionsBuffer(0)
  , mQuadTexCoordsBuffer(0)
  , mQuadElementsBuffer(0)
{
  initContext();
}

RenderContext::~RenderContext()
{
  if (mQuadPositionsBuffer) {
    glDeleteBuffers(1, &mQuadPositionsBuffer);
    mQuadPositionsBuffer = 0;
  }
  if (mQuadTexCoordsBuffer) {
    glDeleteBuffers(1, &mQuadTexCoordsBuffer);
    mQuadTexCoordsBuffer = 0;
  }
  if (mQuadElementsBuffer) {
    glDeleteBuffers(1, &mQuadElementsBuffer);
    mQuadElementsBuffer = 0;
  }
}

void
RenderContext::initQuadVAO(std::map<std::string, GLint>& attributes)
{
  assert(mQuadPositionsBuffer != 0);
  assert(mQuadTexCoordsBuffer != 0);
  assert(mQuadElementsBuffer != 0);
  glBindBuffer(GL_ARRAY_BUFFER, mQuadPositionsBuffer);
  glVertexAttribPointer(attributes["aPosition"], 2, GL_FLOAT, GL_FALSE, 0, 0);
  glBindBuffer(GL_ARRAY_BUFFER, mQuadTexCoordsBuffer);
  glVertexAttribPointer(attributes["aTexCoord"], 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(attributes["aPosition"]);
  glEnableVertexAttribArray(attributes["aTexCoord"]);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mQuadElementsBuffer);
}

void
RenderContext::initContext()
{
  // Upload quad buffers.
  glCreateBuffers(1, &mQuadPositionsBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, mQuadPositionsBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD_POSITIONS), QUAD_POSITIONS, GL_STATIC_DRAW);

  glCreateBuffers(1, &mQuadTexCoordsBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, mQuadTexCoordsBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD_TEX_COORDS), QUAD_TEX_COORDS, GL_STATIC_DRAW);

  glCreateBuffers(1, &mQuadElementsBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mQuadElementsBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(QUAD_ELEMENTS), QUAD_ELEMENTS, GL_STATIC_DRAW);
}

} // namespace pathfinder
