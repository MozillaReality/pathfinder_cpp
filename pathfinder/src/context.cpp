// pathfinder/src/render-context.cpp
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include "context.h"
#include "gl-utils.h"
#include "shader-loader.h"
#include "resources/gamma_lut.h"
#include "resources/area_lut.h"

#include <string>

const int MAX_VERTICES = 4 * 1024 * 1024;
const int MAX_PATHS = 65535;

using namespace std;

namespace pathfinder {

RenderContext::RenderContext()
  : mQuadPositionsBuffer(0)
  , mQuadTexCoordsBuffer(0)
  , mQuadElementsBuffer(0)
  , mGammaLUTTexture(0)
  , mAreaLUTTexture(0)
  , mVertexIDVBO(0)
  , mInstancedPathIDVBO(0)
{
  mShaderManager = make_unique<ShaderManager>();
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
  if (mGammaLUTTexture) {
    glDeleteTextures(1, &mGammaLUTTexture);
    mGammaLUTTexture = 0;
  }
  if (mAreaLUTTexture) {
    glDeleteTextures(1, &mAreaLUTTexture);
    mAreaLUTTexture = 0;
  }
  if (mVertexIDVBO) {
    glDeleteBuffers(1, &mVertexIDVBO);
    mVertexIDVBO = 0;
  }
  if (mInstancedPathIDVBO) {
    glDeleteBuffers(1, &mInstancedPathIDVBO);
    mInstancedPathIDVBO = 0;
  }
}

bool
RenderContext::init()
{
  if (!initContext()) {
    return false;
  }
  if (!mShaderManager->init()) {
    return false;
  }
  if (!initGammaLUTTexture()) {
    return false;
  }
  if (!initAreaLUTTexture()) {
    return false;
  }
  if (!initVertexIDVBO()) {
    return false;
  }
  if (!initVertexIDVBO()) {
    return false;
  }
  if (!initInstancedPathIDVBO()) {
    return false;
  }
  return true;
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

bool
RenderContext::initContext()
{
  // TODO(kearwood) - Error handling

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

  return true;
}

bool
RenderContext::initGammaLUTTexture()
{
  // TODO(kearwood) - Error handling
  GLDEBUG(glCreateTextures(GL_TEXTURE_2D, 1, &mGammaLUTTexture));
  glBindTexture(GL_TEXTURE_2D, mGammaLUTTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, gamma_lut_width, gamma_lut_height, 0, GL_RED, GL_UNSIGNED_BYTE, gamma_lut_raw);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  return true;
}

bool
RenderContext::initAreaLUTTexture()
{
  // TODO(kearwood) - Error handling
  GLDEBUG(glCreateTextures(GL_TEXTURE_2D, 1, &mAreaLUTTexture));
  glBindTexture(GL_TEXTURE_2D, mAreaLUTTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, area_lut_width, area_lut_height, 0, GL_RED, GL_UNSIGNED_BYTE, area_lut_raw);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  return true;
}

bool
RenderContext::initVertexIDVBO()
{
  // TODO(kearwood) - Error handling
  __uint16_t *vertexIDs = new __uint16_t[MAX_VERTICES];
  for (int vertexID = 0; vertexID < MAX_VERTICES; vertexID++) {
    vertexIDs[vertexID] = vertexID;
  }

  GLDEBUG(glCreateBuffers(1, &mVertexIDVBO));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, mVertexIDVBO));
  GLDEBUG(glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(__uint16_t), vertexIDs, GL_STATIC_DRAW));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, 0));

  delete[] vertexIDs;

  return true;
}

bool
RenderContext::initInstancedPathIDVBO()
{
  // TODO(kearwood) - Error handling
  __uint16_t *pathIDs = new __uint16_t[MAX_PATHS];
  for (int pathIndex = 0; pathIndex < MAX_PATHS; pathIndex++) {
   pathIDs[pathIndex] = pathIndex + 1;
  }

  GLDEBUG(glCreateBuffers(1, &mInstancedPathIDVBO));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, mInstancedPathIDVBO));
  GLDEBUG(glBufferData(GL_ARRAY_BUFFER, MAX_PATHS * sizeof(__uint16_t), pathIDs, GL_STATIC_DRAW));
  GLDEBUG(glBindBuffer(GL_ARRAY_BUFFER, 0));

  delete[] pathIDs;

  return true;
}

ColorAlphaFormat
RenderContext::getColorAlphaFormat() const
{
  // On macOS, RGBA framebuffers seem to cause driver stalls when switching between rendering
  // and texturing. Work around this by using RGB5A1 instead.

  return caf_RGBA8;

  // TODO(kearwood) - Do we still need to return caf_RGB5_A1 for macOS in native code?
}



} // namespace pathfinder
