#ifndef PATHFINDER_SHADER_LOADER_H
#define PATHFINDER_SHADER_LOADER_H

namespace pathfinder {

typedef enum {
    shader_blitLinear = 0,
    shader_blitGamma,
    shader_conservativeInterior,
    shader_demo3DDistantGlyph,
    shader_demo3DMonument,
    shader_directCurve,
    shader_directInterior,
    shader_direct3DCurve,
    shader_direct3DInterior,
    shader_mcaa,
    shader_ssaaSubpixelResolve,
    shader_stencilAAA,
    shader_xcaaMonoResolve,
    shader_xcaaMonoSubpixelResolve,
    shader_NumShaders
} ShaderName_t;

/*
template <class T>
struct ShaderMap<T> {
    T blitLinear;
    T blitGamma;
    T conservativeInterior;
    T demo3DDistantGlyph;
    T demo3DMonument;
    T directCurve;
    T directInterior;
    T direct3DCurve;
    T direct3DInterior;
    T mcaa;
    T ssaaSubpixelResolve;
    T stencilAAA;
    T xcaaMonoResolve;
    T xcaaMonoSubpixelResolve;
};
*/

} // namespace pathfinder

#endif // PATHFINDER_SHADER_LOADER_H
