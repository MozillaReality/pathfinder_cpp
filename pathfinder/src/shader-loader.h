// pathfinder/src/shader-loader.h
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
#ifndef PATHFINDER_SHADER_LOADER_H
#define PATHFINDER_SHADER_LOADER_H

#include "resources.h"
#include "gl-utils.h"

#include <map>
#include <string>
#include <vector>
#include <memory>

namespace pathfinder {

#define VERTEX_SHADER_LIST \
SHADER_ITEM(blit) \
SHADER_ITEM(conservative_interior) \
SHADER_ITEM(direct_curve) \
SHADER_ITEM(direct_interior) \
SHADER_ITEM(direct_3d_curve) \
SHADER_ITEM(direct_3d_interior) \
SHADER_ITEM(mcaa) \
SHADER_ITEM(stencil_aaa) \
SHADER_ITEM(xcaa_mono_resolve) \
SHADER_ITEM(xcaa_mono_subpixel_resolve)

#define FRAGMENT_SHADER_LIST \
SHADER_ITEM(blit_gamma) \
SHADER_ITEM(blit_linear) \
SHADER_ITEM(direct_curve) \
SHADER_ITEM(direct_interior) \
SHADER_ITEM(mcaa) \
SHADER_ITEM(ssaa_subpixel_resolve) \
SHADER_ITEM(stencil_aaa) \
SHADER_ITEM(xcaa_mono_resolve) \
SHADER_ITEM(xcaa_mono_subpixel_resolve)

#define PROGRAM_LIST \
PROGRAM_ITEM(blitLinear,              blit_linear,                blit) \
PROGRAM_ITEM(blitGamma,               blit_gamma,                 blit) \
PROGRAM_ITEM(conservativeInterior,    direct_interior,            conservative_interior) \
PROGRAM_ITEM(directCurve,             direct_curve,               direct_curve) \
PROGRAM_ITEM(directInterior,          direct_interior,            direct_interior) \
PROGRAM_ITEM(direct3DCurve,           direct_curve,               direct_3d_curve) \
PROGRAM_ITEM(direct3DInterior,        direct_interior,            direct_3d_interior) \
PROGRAM_ITEM(mcaa,                    mcaa,                       mcaa) \
PROGRAM_ITEM(ssaaSubpixelResolve,     ssaa_subpixel_resolve,      blit) \
PROGRAM_ITEM(stencilAAA,              stencil_aaa,                stencil_aaa) \
PROGRAM_ITEM(xcaaMonoResolve,         xcaa_mono_resolve,          xcaa_mono_resolve) \
PROGRAM_ITEM(xcaaMonoSubpixelResolve, xcaa_mono_subpixel_resolve, xcaa_mono_subpixel_resolve)

#define UNIFORM_LIST \
UNIFORM_ITEM(uAAAlpha) \
UNIFORM_ITEM(uAAAlphaDimensions) \
UNIFORM_ITEM(uAreaLUT) \
UNIFORM_ITEM(uBGColor) \
UNIFORM_ITEM(uEmboldenAmount) \
UNIFORM_ITEM(uFGColor) \
UNIFORM_ITEM(uFramebufferSize) \
UNIFORM_ITEM(uGammaLUT) \
UNIFORM_ITEM(uHints) \
UNIFORM_ITEM(uKernel) \
UNIFORM_ITEM(uMulticolor) \
UNIFORM_ITEM(uPathBounds) \
UNIFORM_ITEM(uPathBoundsDimensions) \
UNIFORM_ITEM(uPathColors) \
UNIFORM_ITEM(uPathColorsDimensions) \
UNIFORM_ITEM(uPathTransformExt) \
UNIFORM_ITEM(uPathTransformExtDimensions) \
UNIFORM_ITEM(uPathTransformST) \
UNIFORM_ITEM(uPathTransformSTDimensions) \
UNIFORM_ITEM(uSide) \
UNIFORM_ITEM(uSource) \
UNIFORM_ITEM(uSourceDimensions) \
UNIFORM_ITEM(uTexScale) \
UNIFORM_ITEM(uTransform) \
UNIFORM_ITEM(uTransformExt) \
UNIFORM_ITEM(uTransformST)

#define ATTRIBUTE_LIST \
ATTRIBUTE_ITEM(aCtrlNormal) \
ATTRIBUTE_ITEM(aCtrlPosition) \
ATTRIBUTE_ITEM(aDUVDX) \
ATTRIBUTE_ITEM(aDUVDY) \
ATTRIBUTE_ITEM(aFromNormal) \
ATTRIBUTE_ITEM(aFromPosition) \
ATTRIBUTE_ITEM(aNormalAngle) \
ATTRIBUTE_ITEM(aPathID) \
ATTRIBUTE_ITEM(aPosition) \
ATTRIBUTE_ITEM(aRect) \
ATTRIBUTE_ITEM(aSignMode) \
ATTRIBUTE_ITEM(aTessCoord) \
ATTRIBUTE_ITEM(aTexCoord) \
ATTRIBUTE_ITEM(aToNormal) \
ATTRIBUTE_ITEM(aToPosition) \
ATTRIBUTE_ITEM(aUV) \
ATTRIBUTE_ITEM(aVertexID)


typedef enum {
#define UNIFORM_ITEM(name) \
  uniform_ ## name ,
UNIFORM_LIST
#undef UNIFORM_ITEM
  uniform_count
} UniformID;

static const char* UNIFORM_NAMES[] {
#define UNIFORM_ITEM(name) \
  #name ,
UNIFORM_LIST
#undef UNIFORM_ITEM
};

typedef enum {
#define ATTRIBUTE_ITEM(name) \
  attribute_ ## name ,
  ATTRIBUTE_LIST
#undef ATTRIBUTE_ITEM
  attribute_count
} AttributeID;

static const char* ATTRIBUTE_NAMES[]{
#define ATTRIBUTE_ITEM(name) \
  #name ,
  ATTRIBUTE_LIST
#undef ATTRIBUTE_ITEM
};

typedef enum {
#define SHADER_ITEM(name) \
  vs_ ## name ,
VERTEX_SHADER_LIST
#undef SHADER_ITEM
  vs_count
} VertexShaderID;

typedef enum {
#define SHADER_ITEM(name) \
  fs_ ## name ,
FRAGMENT_SHADER_LIST
#undef SHADER_ITEM
  fs_count
} FragmentShaderID;

typedef enum {
#define PROGRAM_ITEM(name, frag_src, vert_src) \
  program_ ## name ,
PROGRAM_LIST
#undef PROGRAM_ITEM
  program_count
} ProgramID;

static const char* VertexShaderSource[] {
#define SHADER_ITEM(name) \
  shader_ ## name ## _vs,
VERTEX_SHADER_LIST
#undef SHADER_ITEM
};

static const char* FragmentShaderSource[] {
#define SHADER_ITEM(name) \
  shader_ ## name ## _fs,
FRAGMENT_SHADER_LIST
#undef SHADER_ITEM
};


static const char* VertexShaderNames[] {
#define SHADER_ITEM(name) \
  #name ,
VERTEX_SHADER_LIST
#undef SHADER_ITEM
};

static const char* FragmentShaderNames[] {
#define SHADER_ITEM(name) \
  #name ,
FRAGMENT_SHADER_LIST
#undef SHADER_ITEM
};

struct ProgramInfo {
  const char* name;
  GLuint vertex;
  GLuint fragment;
};

static const ProgramInfo PROGRAM_INFO[] = {
#define PROGRAM_ITEM(name, frag_src, vert_src) \
{ \
    #name, \
    vs_ ## vert_src , \
    fs_ ## frag_src \
},
PROGRAM_LIST
#undef PROGRAM_ITEM
};

#undef PROGRAM_LIST
#undef FRAGMENT_SHADER_LIST
#undef VERTEX_SHADER_LIST

class PathfinderShaderProgram;

class ShaderManager
{
public:
  ShaderManager();
  ~ShaderManager();
  ShaderManager(const ShaderManager&) = delete;
  ShaderManager& operator=(const ShaderManager&) = delete;

  bool init();

  std::shared_ptr<PathfinderShaderProgram> getProgram(ProgramID aProgramID);

private:
  std::vector<std::shared_ptr<PathfinderShaderProgram>> mPrograms;
  GLuint loadShader(const char* aName, const char* aSource, GLenum aType);
}; // class ShaderManager

class PathfinderShaderProgram
{
public:
  PathfinderShaderProgram();
  ~PathfinderShaderProgram();
  PathfinderShaderProgram(const PathfinderShaderProgram&) = delete;
  PathfinderShaderProgram& operator=(const PathfinderShaderProgram&) = delete;
  bool load(const char* aProgramName,
            GLuint aVertexShader,
            GLuint aFragmentShader);
  GLuint getProgram() {
    return mProgram;
  }
  std::string getProgramName() {
    return mProgramName;
  }
  GLint getUniform(UniformID aUniformID);
  bool hasUniform(UniformID aUniformID);
  GLint getAttribute(AttributeID aAttributeID);
  bool hasAttribute(AttributeID aAttributeID);
private:
  GLuint mProgram;
  std::string mProgramName;
  GLint mAttributes[attribute_count];
  GLint mUniforms[uniform_count];
};


} // namespace pathfinder

#endif // PATHFINDER_SHADER_LOADER_H
