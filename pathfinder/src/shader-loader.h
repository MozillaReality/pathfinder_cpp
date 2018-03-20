#ifndef PATHFINDER_SHADER_LOADER_H
#define PATHFINDER_SHADER_LOADER_H

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

#endif // PATHFINDER_SHADER_LOADER_H
