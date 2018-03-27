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

#include <string>

namespace pathfinder {

RenderContext::RenderContext()
  : mQuadPositionsBuffer(0)
  , mQuadTexCoordsBuffer(0)
  , mQuadElementsBuffer(0)
{

}

void
RenderContext::initQuadVAO(std::map<std::string, GLint>& attributes)
{
  glBindBuffer(GL_ARRAY_BUFFER, quadPositionsBuffer());
  glVertexAttribPointer(attributes["aPosition"], 2, GL_FLOAT, GL_FALSE, 0, 0);
  glBindBuffer(GL_ARRAY_BUFFER, quadTexCoordsBuffer());
  glVertexAttribPointer(attributes["aTexCoord"], 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(attributes["aPosition"]);
  glEnableVertexAttribArray(attributes["aTexCoord"]);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadElementsBuffer());
}

} // namespace pathfinder
