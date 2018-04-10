// pathfinder/src/render-context.h
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#ifndef PATHFINDER_RENDER_CONTEXT_H
#define PATHFINDER_RENDER_CONTEXT_H

#include "platform.h"

#include "gl-utils.h"

#include <vector>
#include <memory>

namespace pathfinder {

class PathfinderShaderProgram;
class ShaderManager;

class RenderContext
{
public:
  RenderContext();
  ~RenderContext();
  virtual bool init();
  void initQuadVAO(std::map<std::string, GLint>& attributes);

  virtual ColorAlphaFormat getColorAlphaFormat() const = 0;

  ShaderManager& getShaderManager() {
    assert(mShaderManager);
    return *mShaderManager;
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
protected:
  bool initContext();
    
private:
  std::unique_ptr<ShaderManager> mShaderManager;
  GLuint mQuadPositionsBuffer;
  GLuint mQuadTexCoordsBuffer;
  GLuint mQuadElementsBuffer;
};

} // namespace pathfinder

#endif // PATHFINDER_RENDER_CONTEXT_H
