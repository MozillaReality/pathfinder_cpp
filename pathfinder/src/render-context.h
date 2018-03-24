#ifndef PATHFINDER_RENDER_CONTEXT_H
#define PATHFINDER_RENDER_CONTEXT_H

#include "platform.h"

#include "gl-utils.h"

#include <vector>
#include <memory>

namespace pathfinder {

class PathfinderShaderProgram;

class RenderContext
{
public:
  RenderContext();
  void initQuadVAO(std::map<std::string, GLint>& attributes);

  ColorAlphaFormat getColorAlphaFormat() const {
    return mColorAlphaFormat;
  }

  const std::vector<std::shared_ptr<PathfinderShaderProgram>>& shaderPrograms() {
    return mShaderPrograms;
  }

  GLuint quadPositionsBuffer() {
    return mQuadPositionsBuffer;
  }
  GLuint quadElementsBuffer() {
    return mQuadElementsBuffer;
  }
  GLuint quadTexCoordsBuffer() {
    return mQuadTexCoordsBuffer;
  }

  virtual void setDirty() = 0;
    
private:
    //     /// The OpenGL context.
  
    //readonly gl: WebGLRenderingContext;
    ColorAlphaFormat mColorAlphaFormat;
    std::vector<std::shared_ptr<PathfinderShaderProgram>> mShaderPrograms;
    GLuint mQuadPositionsBuffer;
    GLuint mQuadTexCoordsBuffer;
    GLuint mQuadElementsBuffer;
};

} // namespace pathfinder

#endif // PATHFINDER_RENDER_CONTEXT_H
