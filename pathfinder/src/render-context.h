#ifndef PATHFINDER_RENDER_CONTEXT_H
#define PATHFINDER_RENDER_CONTEXT_H

#include <GL/glew.h>

namespace pathfinder {

class RenderContext
{
public:
    /// The OpenGL context.
  /*
    readonly gl: WebGLRenderingContext;

    readonly instancedArraysExt: ANGLEInstancedArrays;
    readonly textureHalfFloatExt: OESTextureHalfFloat;
    readonly timerQueryExt: EXTDisjointTimerQuery;
    readonly vertexArrayObjectExt: OESVertexArrayObject;

    readonly colorAlphaFormat: ColorAlphaFormat;

    readonly shaderPrograms: ShaderMap<PathfinderShaderProgram>;
    readonly gammaLUT: HTMLImageElement;

    readonly quadPositionsBuffer: WebGLBuffer;
    readonly quadElementsBuffer: WebGLBuffer;

    readonly atlasRenderingTimerQuery: WebGLQuery;
    readonly compositingTimerQuery: WebGLQuery;

    initQuadVAO(attributes: any): void;
*/
    virtual void setDirty() = 0;
};

} // namespace pathfinder

#endif // PATHFINDER_RENDER_CONTEXT_H
