#include "render-context.h"

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
