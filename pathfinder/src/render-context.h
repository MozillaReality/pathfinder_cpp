#ifndef PATHFINDER_RENDER_CONTEXT_H
#define PATHFINDER_RENDER_CONTEXT_H

#include <GL/glew.h>

namespace pathfinder {

class RenderContext
{
public:
    /*
    // OpenGL Extensions
    readonly instancedArraysExt: ANGLEInstancedArrays;
    readonly textureHalfFloatExt: OESTextureHalfFloat;
    readonly timerQueryExt: EXTDisjointTimerQuery;
    readonly vertexArrayObjectExt: OESVertexArrayObject;

    void initQuadVAO(attributes: any);

    readonly gammaLUT: HTMLImageElement;
    */

    ColorAlphaFormat getColorAlphaFormat() const;

    const std::vector<std::shared_ptr<PathfinderShaderProgram>>& shaderPrograms() {
        return mShaderPrograms;
    }

    GLuint quadPositionsBuffer() {
        return mQuadPositionsBuffer;
    }
    GLuint quadElementsBuffer() {
        return mQuadElementsBuffer;
    }

    virtual void setDirty() = 0;
    
private:
    //     /// The OpenGL context.
  
    //readonly gl: WebGLRenderingContext;
    ColorAlphaFormat mColorAlphaFormat;
    std::vector<std::shared_ptr<PathfinderShaderProgram>> mShaderPrograms;
    GLuint mQuadPositionsBuffer;
    GLuint mQuadElementsBuffer;
};

} // namespace pathfinder

#endif // PATHFINDER_RENDER_CONTEXT_H
