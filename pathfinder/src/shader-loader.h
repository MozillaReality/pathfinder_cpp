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
  bool load(const char* aProgramName,
            GLuint aVertexShader,
            GLuint aFragmentShader);
  GLuint getProgram() {
    return mProgram;
  }
  std::string getProgramName() {
    return mProgramName;
  }
  UniformMap& getUniforms()
  {
    return mUniforms;
  }
  std::map<std::string, GLint>& getAttributes()
  {
    return mAttributes;
  }

private:
  GLuint mProgram;
  std::string mProgramName;
  UniformMap mUniforms;
  std::map<std::string, GLint> mAttributes;
};


} // namespace pathfinder

#endif // PATHFINDER_SHADER_LOADER_H
