#ifndef PATHFINDER_SHADER_LOADER_H
#define PATHFINDER_SHADER_LOADER_H

#include "resources.h"
#include "gl-utils.h"

#include <map>
#include <string>

namespace pathfinder {

#define SHADER_LIST \
SHADER_ITEM(blitLinear,              blit_linear_fs,                blit_vs) \
SHADER_ITEM(blitGamma,               blit_gamma_fs,                 blit_vs) \
SHADER_ITEM(conservativeInterior,    direct_interior_fs,            conservative_interior_vs) \
SHADER_ITEM(directCurve,             direct_curve_fs,               direct_curve_vs) \
SHADER_ITEM(directInterior,          direct_interior_fs,            direct_interior_vs) \
SHADER_ITEM(direct3DCurve,           direct_curve_fs,               direct_3d_curve_vs) \
SHADER_ITEM(direct3DInterior,        direct_interior_fs,            direct_3d_interior_vs) \
SHADER_ITEM(mcaa,                    mcaa_fs,                       mcaa_vs) \
SHADER_ITEM(ssaaSubpixelResolve,     ssaa_subpixel_resolve_fs,      blit_vs) \
SHADER_ITEM(stencilAAA,              stencil_aaa_fs,                stencil_aaa_vs) \
SHADER_ITEM(xcaaMonoResolve,         xcaa_mono_resolve_fs,          xcaa_mono_resolve_vs) \
SHADER_ITEM(xcaaMonoSubpixelResolve, xcaa_mono_subpixel_resolve_fs, xcaa_mono_subpixel_resolve_vs)

typedef enum {
#define SHADER_ITEM(name, frag_src, vert_src) \
    shader_ ## name ,
SHADER_LIST
#undef SHADER_ITEM
    shader_NumShaders
} ShaderID;

static const char* SHADER_NAMES[] = {
#define SHADER_ITEM(name, frag_src, vert_src) \
    "name",
SHADER_LIST
#undef SHADER_ITEM
};

struct UnlinkedShaderProgram {
  GLuint vertex;
  GLuint fragment;
};

struct ShaderProgramSource {
  const char* vertex;
  const char* fragment;
};

const ShaderProgramSource SHADER_SOURCE[] = {
#define SHADER_ITEM(name, frag_src, vert_src) \
  { shader_ ## vert_src , shader_ ## frag_src },
SHADER_LIST
#undef SHADER_ITEM
};

#undef SHADER_LIST


class PathfinderShaderProgram
{
public:
  PathfinderShaderProgram(const std::string& aProgramName,
                            const UnlinkedShaderProgram& aUnlinkedShaderProgram);
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
