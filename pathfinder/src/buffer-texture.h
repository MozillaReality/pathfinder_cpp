// pathfinder/src/biuffer-texture.h
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#ifndef PATHFINDER_BUFFER_TEXTURE_H
#define PATHFINDER_BUFFER_TEXTURE_H

#include "gl-utils.h"

#include "platform.h"
#include <kraken-math.h>
#include <vector>
#include <memory>
#include <string>

namespace pathfinder {

class PathfinderShaderProgram;

class PathfinderBufferTexture
{
public:
  PathfinderBufferTexture(const std::string& aUniformName);
  void destroy();

  GLuint getTexture() const;
  GLuint getUniformName() const;
  void upload(const float *data, GLsizei length);
  void upload(const std::vector<float>& data);
  void upload(const std::vector<__uint8_t>& data);
  void bind(PathfinderShaderProgram& uniforms, GLuint textureUnit);

private:
  GLuint mTexture;
  std::string mUniformName;
  GLsizei mSideLength;
  GLuint mGLType;
  bool mDestroyed;

  void upload(__uint8_t* data, GLsizei length, GLuint glType);
  GLsizei getArea();
};

} // namespace pathfinder

#endif // PATHFINDER_BUFFER_TEXTURE_H
