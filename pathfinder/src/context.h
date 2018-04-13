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
#include <assert.h>

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

  ColorAlphaFormat getColorAlphaFormat() const;

  ShaderManager& getShaderManager() {
    assert(mShaderManager);
    return *mShaderManager;
  }

  GLuint quadPositionsBuffer() {
    assert(mQuadPositionsBuffer);
    return mQuadPositionsBuffer;
  }
  GLuint quadElementsBuffer() {
    assert(mQuadElementsBuffer);
    return mQuadElementsBuffer;
  }
  GLuint quadTexCoordsBuffer() {
    assert(mQuadTexCoordsBuffer);
    return mQuadTexCoordsBuffer;
  }
  GLuint getGammaLUTTexture() {
    assert(mGammaLUTTexture);
    return mGammaLUTTexture;
  }
  GLuint getAreaLUTTexture() {
    assert(mAreaLUTTexture);
    return mAreaLUTTexture;
  }
  GLuint getVertexIDVBO() {
    assert(mVertexIDVBO);
    return mVertexIDVBO;
  }
  GLuint getInstancedPathIDVBO() {
    assert(mInstancedPathIDVBO);
    return mInstancedPathIDVBO;
  }
protected:
  bool initContext();
  bool initGammaLUTTexture();
  bool initAreaLUTTexture();
  bool initVertexIDVBO();
  bool initInstancedPathIDVBO();
    
private:
  std::unique_ptr<ShaderManager> mShaderManager;
  GLuint mQuadPositionsBuffer;
  GLuint mQuadTexCoordsBuffer;
  GLuint mQuadElementsBuffer;
  GLuint mGammaLUTTexture;
  GLuint mAreaLUTTexture;
  GLuint mVertexIDVBO;
  GLuint mInstancedPathIDVBO;
};

} // namespace pathfinder

#endif // PATHFINDER_RENDER_CONTEXT_H
