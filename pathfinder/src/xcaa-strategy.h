#ifndef PATHFINDER_XCAA_STRATEGY_H
#define PATHFINDER_XCAA_STRATEGY_H

#include "aa-strategy.h"
#include "platform.h"
#include "buffer-texture.h"
#include "shader-loader.h"
#include "render-context.h"

#include <vector>
#include <map>
#include <memory>
#include <kraken-math.h>

namespace pathfinder {

struct FastEdgeVAOs {
  GLuint upper;
  GLuint lower;
};

/*

type Direction = 'upper' | 'lower';

const DIRECTIONS: Direction[] = ['upper', 'lower'];
*/

const float PATCH_VERTICES[] = {
  0.0f, 0.0f,
  0.5f, 0.0f,
  1.0f, 0.0f,
  0.0f, 1.0f,
  0.5f, 1.0f,
  1.0f, 1.0f,
};
const GLsizei PATCH_VERTICES_SIZE = sizeof(float) * 12;

const __uint8_t MCAA_PATCH_INDICES[] = {
  0, 1, 2, 1, 3, 2
};
const GLsizei MCAA_PATCH_INDICES_SIZE = sizeof(__uint8_t) * 6;

typedef enum {
  tt_affine,
  tt_3d
} TransformType;

class XCAAStrategy : public AntialiasingStrategy {
public:
  XCAAStrategy(int aLevel, SubpixelAAType aSubpixelAA);

  int getPassCount() const override {
    return 1;
  }

  virtual void attachMeshes(RenderContext& renderContext, Renderer& renderer) override;
  virtual void setFramebufferSize(Renderer& renderer) override;
  virtual void finishAntialiasingObject(Renderer& renderer, int objectIndex) override;
  virtual void finishDirectlyRenderingObject(Renderer& renderer, int objectIndex) override;
  virtual void antialiasObject(Renderer& renderer, int objectIndex) override;
  virtual void resolveAAForObject(Renderer& renderer, int objectIndex) override;
  virtual void resolve(int pass, Renderer& renderer) override { }
  virtual kraken::Matrix4 getTransform() const override;
 
protected:
  virtual TransformType getTransformType() const = 0;
  virtual bool getMightUseAAFramebuffer() const = 0;
  virtual bool usesAAFramebuffer(Renderer& renderer) = 0;
  kraken::Vector2i supersampledUsedSize(Renderer& renderer);
  void prepareAA(Renderer& renderer);
  void setAAState(Renderer& renderer);
  void setAAUniforms(Renderer& renderer, UniformMap& uniforms, int objectIndex);
  void setDepthAndBlendModeForResolve();
  void setAdditionalStateForResolveIfNecessary(Renderer& renderer,
                                               PathfinderShaderProgram& resolveProgram,
                                               int firstFreeTextureUnit) { }
  virtual void clearForAA(Renderer& renderer) = 0;
  virtual bool usesResolveProgram(Renderer& renderer) = 0;
  virtual PathfinderShaderProgram& getResolveProgram(Renderer& renderer) = 0;
  virtual void setAADepthState(Renderer& renderer) = 0;
  virtual void clearForResolve(Renderer& renderer) = 0;
  GLuint getDirectDepthTexture()
  {
    return 0;
  }
  kraken::Vector2i getSupersampleScale() {
    return kraken::Vector2i::Create(mSubpixelAA != saat_none ? 3 : 1, 1);
  }

  GLuint patchVertexBuffer;
  GLuint patchIndexBuffer;
  std::map<int, std::unique_ptr<PathfinderBufferTexture>> pathBoundsBufferTextures;
  kraken::Vector2i supersampledFramebufferSize;
  kraken::Vector2i destFramebufferSize;
  GLuint resolveVAO;
  GLuint aaAlphaTexture;
  GLuint aaDepthTexture;
  GLuint aaFramebuffer;

private:
  void initResolveFramebufferForObject(Renderer& renderer, int objectIndex);
  void initAAAlphaFramebuffer(Renderer& renderer);
  void createPathBoundsBufferTextureForObjectIfNecessary(Renderer& renderer, int objectIndex);
  void createResolveVAO(RenderContext& renderContext, Renderer& renderer);
}; // class XCAAStrategy

/*
export class MCAAStrategy extends XCAAStrategy {
    protected vao: WebGLVertexArrayObject | null;

    protected get transformType(): TransformType {
        return 'affine';
    }

    protected get mightUseAAFramebuffer(): boolean {
        return true;
    }

    attachMeshes(renderer: Renderer): void {
        super.attachMeshes(renderer);

        const renderContext = renderer.renderContext;
        const gl = renderContext.gl;

        this.vao = renderContext.vertexArrayObjectExt.createVertexArrayOES();
    }

    antialiasObject(renderer: Renderer, objectIndex: number): void {
        super.antialiasObject(renderer, objectIndex);

        const shaderProgram = this.edgeProgram(renderer);
        this.antialiasEdgesOfObjectWithProgram(renderer, objectIndex, shaderProgram);
    }

    protected usesAAFramebuffer(renderer: Renderer): boolean {
        return !renderer.isMulticolor;
    }

    virtual bool usesResolveProgram(Renderer& renderer) override
    {
      return !renderer.isMulticolor;
    }

    protected getResolveProgram(renderer: Renderer): PathfinderShaderProgram | null {
        const renderContext = renderer.renderContext;
        assert(!renderer.isMulticolor);
        if (this.subpixelAA !== 'none' && renderer.allowSubpixelAA)
            return renderContext.shaderPrograms.xcaaMonoSubpixelResolve;
        return renderContext.shaderPrograms.xcaaMonoResolve;
    }

    protected clearForAA(renderer: Renderer): void {
        if (!this.usesAAFramebuffer(renderer))
            return;

        const renderContext = renderer.renderContext;
        const gl = renderContext.gl;

        gl.clearColor(0.0, 0.0, 0.0, 0.0);
        gl.clearDepth(0.0);
        gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    }

    protected setAADepthState(renderer: Renderer): void {
        const renderContext = renderer.renderContext;
        const gl = renderContext.gl;

        if (this.directRenderingMode !== 'conservative') {
            gl.disable(gl.DEPTH_TEST);
            return;
        }

        gl.depthFunc(gl.GREATER);
        gl.depthMask(false);
        gl.enable(gl.DEPTH_TEST);
        gl.disable(gl.CULL_FACE);
    }

    protected clearForResolve(renderer: Renderer): void {
        const renderContext = renderer.renderContext;
        const gl = renderContext.gl;

        if (!renderer.isMulticolor) {
            gl.clearColor(0.0, 0.0, 0.0, 1.0);
            gl.clear(gl.COLOR_BUFFER_BIT);
        }
    }

    protected setBlendModeForAA(renderer: Renderer): void {
        const renderContext = renderer.renderContext;
        const gl = renderContext.gl;

        if (renderer.isMulticolor)
            gl.blendFuncSeparate(gl.ONE, gl.ONE_MINUS_SRC_ALPHA, gl.ONE, gl.ONE);
        else
            gl.blendFunc(gl.ONE, gl.ONE);

        gl.blendEquation(gl.FUNC_ADD);
        gl.enable(gl.BLEND);
    }

    protected prepareAA(renderer: Renderer): void {
        super.prepareAA(renderer);

        this.setBlendModeForAA(renderer);
    }

    protected initVAOForObject(renderer: Renderer, objectIndex: number): void {
        if (renderer.meshBuffers == null || renderer.meshes == null)
            return;

        const renderContext = renderer.renderContext;
        const gl = renderContext.gl;

        const pathRange = renderer.pathRangeForObject(objectIndex);
        const meshIndex = renderer.meshIndexForObject(objectIndex);

        const shaderProgram = this.edgeProgram(renderer);
        const attributes = shaderProgram.attributes;

        // FIXME(pcwalton): Refactor.
        const vao = this.vao;
        renderContext.vertexArrayObjectExt.bindVertexArrayOES(vao);

        const bBoxRanges = renderer.meshes[meshIndex].bBoxPathRanges;
        const offset = calculateStartFromIndexRanges(pathRange, bBoxRanges);

        gl.useProgram(shaderProgram.program);
        gl.bindBuffer(gl.ARRAY_BUFFER, renderer.renderContext.quadPositionsBuffer);
        gl.vertexAttribPointer(attributes.aTessCoord, 2, gl.FLOAT, false, FLOAT32_SIZE * 2, 0);
        gl.bindBuffer(gl.ARRAY_BUFFER, renderer.meshBuffers[meshIndex].bBoxes);
        gl.vertexAttribPointer(attributes.aRect,
                               4,
                               gl.FLOAT,
                               false,
                               FLOAT32_SIZE * 20,
                               FLOAT32_SIZE * 0 + offset * FLOAT32_SIZE * 20);
        gl.vertexAttribPointer(attributes.aUV,
                               4,
                               gl.FLOAT,
                               false,
                               FLOAT32_SIZE * 20,
                               FLOAT32_SIZE * 4 + offset * FLOAT32_SIZE * 20);
        gl.vertexAttribPointer(attributes.aDUVDX,
                               4,
                               gl.FLOAT,
                               false,
                               FLOAT32_SIZE * 20,
                               FLOAT32_SIZE * 8 + offset * FLOAT32_SIZE * 20);
        gl.vertexAttribPointer(attributes.aDUVDY,
                               4,
                               gl.FLOAT,
                               false,
                               FLOAT32_SIZE * 20,
                               FLOAT32_SIZE * 12 + offset * FLOAT32_SIZE * 20);
        gl.vertexAttribPointer(attributes.aSignMode,
                               4,
                               gl.FLOAT,
                               false,
                               FLOAT32_SIZE * 20,
                               FLOAT32_SIZE * 16 + offset * FLOAT32_SIZE * 20);

        gl.enableVertexAttribArray(attributes.aTessCoord);
        gl.enableVertexAttribArray(attributes.aRect);
        gl.enableVertexAttribArray(attributes.aUV);
        gl.enableVertexAttribArray(attributes.aDUVDX);
        gl.enableVertexAttribArray(attributes.aDUVDY);
        gl.enableVertexAttribArray(attributes.aSignMode);

        renderContext.instancedArraysExt.vertexAttribDivisorANGLE(attributes.aRect, 1);
        renderContext.instancedArraysExt.vertexAttribDivisorANGLE(attributes.aUV, 1);
        renderContext.instancedArraysExt.vertexAttribDivisorANGLE(attributes.aDUVDX, 1);
        renderContext.instancedArraysExt.vertexAttribDivisorANGLE(attributes.aDUVDY, 1);
        renderContext.instancedArraysExt.vertexAttribDivisorANGLE(attributes.aSignMode, 1);

        gl.bindBuffer(gl.ARRAY_BUFFER, renderer.meshBuffers[meshIndex].bBoxPathIDs);
        gl.vertexAttribPointer(attributes.aPathID,
                               1,
                               gl.UNSIGNED_SHORT,
                               false,
                               UINT16_SIZE,
                               offset * UINT16_SIZE);
        gl.enableVertexAttribArray(attributes.aPathID);
        renderContext.instancedArraysExt.vertexAttribDivisorANGLE(attributes.aPathID, 1);

        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, renderer.renderContext.quadElementsBuffer);

        renderContext.vertexArrayObjectExt.bindVertexArrayOES(null);
    }

    protected edgeProgram(renderer: Renderer): PathfinderShaderProgram {
        return renderer.renderContext.shaderPrograms.mcaa;
    }

    protected antialiasEdgesOfObjectWithProgram(renderer: Renderer,
                                                objectIndex: number,
                                                shaderProgram: PathfinderShaderProgram):
                                                void {
        if (renderer.meshBuffers == null || renderer.meshes == null)
            return;

        const renderContext = renderer.renderContext;
        const gl = renderContext.gl;

        const pathRange = renderer.pathRangeForObject(objectIndex);
        const meshIndex = renderer.meshIndexForObject(objectIndex);

        this.initVAOForObject(renderer, objectIndex);

        gl.useProgram(shaderProgram.program);
        const uniforms = shaderProgram.uniforms;
        this.setAAUniforms(renderer, uniforms, objectIndex);

        // FIXME(pcwalton): Refactor.
        const vao = this.vao;
        renderContext.vertexArrayObjectExt.bindVertexArrayOES(vao);

        this.setBlendModeForAA(renderer);
        this.setAADepthState(renderer);

        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, renderContext.quadElementsBuffer);

        const bBoxRanges = renderer.meshes[meshIndex].bBoxPathRanges;
        const count = calculateCountFromIndexRanges(pathRange, bBoxRanges);

        renderContext.instancedArraysExt
                     .drawElementsInstancedANGLE(gl.TRIANGLES, 6, gl.UNSIGNED_BYTE, 0, count);

        renderContext.vertexArrayObjectExt.bindVertexArrayOES(null);
        gl.disable(gl.DEPTH_TEST);
        gl.disable(gl.CULL_FACE);
    }

    get directRenderingMode(): DirectRenderingMode {
        // FIXME(pcwalton): Only in multicolor mode?
        return 'conservative';
    }

    protected setAAUniforms(renderer: Renderer, uniforms: UniformMap, objectIndex: number): void {
        super.setAAUniforms(renderer, uniforms, objectIndex);

        const renderContext = renderer.renderContext;
        const gl = renderContext.gl;

        renderer.setPathColorsUniform(0, uniforms, 3);

        gl.uniform1i(uniforms.uMulticolor, renderer.isMulticolor ? 1 : 0);
    }
}

export class StencilAAAStrategy extends XCAAStrategy {
    directRenderingMode: DirectRenderingMode = 'none';

    protected transformType: TransformType = 'affine';
    protected mightUseAAFramebuffer: boolean = true;

    private vao: WebGLVertexArrayObject;

    attachMeshes(renderer: Renderer): void {
        super.attachMeshes(renderer);
        this.createVAO(renderer);
    }

    antialiasObject(renderer: Renderer, objectIndex: number): void {
        super.antialiasObject(renderer, objectIndex);

        const renderContext = renderer.renderContext;
        const gl = renderContext.gl;

        if (renderer.meshes == null)
            return;

        // Antialias.
        const shaderPrograms = renderer.renderContext.shaderPrograms;
        this.setAAState(renderer);
        this.setBlendModeForAA(renderer);

        const program = renderContext.shaderPrograms.stencilAAA;
        gl.useProgram(program.program);
        const uniforms = program.uniforms;
        this.setAAUniforms(renderer, uniforms, objectIndex);

        renderContext.vertexArrayObjectExt.bindVertexArrayOES(this.vao);

        // FIXME(pcwalton): Only render the appropriate instances.
        const count = renderer.meshes[0].count('stencilSegments');
        for (let side = 0; side < 2; side++) {
            gl.uniform1i(uniforms.uSide, side);
            renderContext.instancedArraysExt
                        .drawElementsInstancedANGLE(gl.TRIANGLES, 6, gl.UNSIGNED_BYTE, 0, count);
        }

        renderContext.vertexArrayObjectExt.bindVertexArrayOES(null);
    }

    protected usesAAFramebuffer(renderer: Renderer): boolean {
        return true;
    }

    protected clearForAA(renderer: Renderer): void {
        const renderContext = renderer.renderContext;
        const gl = renderContext.gl;

        gl.clearColor(0.0, 0.0, 0.0, 0.0);
        gl.clearDepth(0.0);
        gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    }

    virtual bool usesResolveProgram(Renderer& renderer) override
    {
      return true;
    }

    protected getResolveProgram(renderer: Renderer): PathfinderShaderProgram | null {
        const renderContext = renderer.renderContext;

        if (this.subpixelAA !== 'none' && renderer.allowSubpixelAA)
            return renderContext.shaderPrograms.xcaaMonoSubpixelResolve;
        return renderContext.shaderPrograms.xcaaMonoResolve;
    }

    protected setAADepthState(renderer: Renderer): void {
        const renderContext = renderer.renderContext;
        const gl = renderContext.gl;

        gl.disable(gl.DEPTH_TEST);
        gl.disable(gl.CULL_FACE);
    }

    protected setAAUniforms(renderer: Renderer, uniforms: UniformMap, objectIndex: number):
                            void {
        super.setAAUniforms(renderer, uniforms, objectIndex);
        renderer.setEmboldenAmountUniform(objectIndex, uniforms);
    }

    protected clearForResolve(renderer: Renderer): void {
        const renderContext = renderer.renderContext;
        const gl = renderContext.gl;

        gl.clearColor(0.0, 0.0, 0.0, 0.0);
        gl.clear(gl.COLOR_BUFFER_BIT);
    }

    private createVAO(renderer: Renderer): void {
        if (renderer.meshBuffers == null || renderer.meshes == null)
            return;

        const renderContext = renderer.renderContext;
        const gl = renderContext.gl;

        const program = renderContext.shaderPrograms.stencilAAA;
        const attributes = program.attributes;

        this.vao = renderContext.vertexArrayObjectExt.createVertexArrayOES();
        renderContext.vertexArrayObjectExt.bindVertexArrayOES(this.vao);

        const vertexPositionsBuffer = renderer.meshBuffers[0].stencilSegments;
        const vertexNormalsBuffer = renderer.meshBuffers[0].stencilNormals;
        const pathIDsBuffer = renderer.meshBuffers[0].stencilSegmentPathIDs;

        gl.useProgram(program.program);
        gl.bindBuffer(gl.ARRAY_BUFFER, renderContext.quadPositionsBuffer);
        gl.vertexAttribPointer(attributes.aTessCoord, 2, gl.FLOAT, false, 0, 0);
        gl.bindBuffer(gl.ARRAY_BUFFER, vertexPositionsBuffer);
        gl.vertexAttribPointer(attributes.aFromPosition, 2, gl.FLOAT, false, FLOAT32_SIZE * 6, 0);
        gl.vertexAttribPointer(attributes.aCtrlPosition,
                               2,
                               gl.FLOAT,
                               false,
                               FLOAT32_SIZE * 6,
                               FLOAT32_SIZE * 2);
        gl.vertexAttribPointer(attributes.aToPosition,
                               2,
                               gl.FLOAT,
                               false,
                               FLOAT32_SIZE * 6,
                               FLOAT32_SIZE * 4);
        gl.bindBuffer(gl.ARRAY_BUFFER, vertexNormalsBuffer);
        gl.vertexAttribPointer(attributes.aFromNormal, 2, gl.FLOAT, false, FLOAT32_SIZE * 6, 0);
        gl.vertexAttribPointer(attributes.aCtrlNormal,
                               2,
                               gl.FLOAT,
                               false,
                               FLOAT32_SIZE * 6,
                               FLOAT32_SIZE * 2);
        gl.vertexAttribPointer(attributes.aToNormal,
                               2,
                               gl.FLOAT,
                               false,
                               FLOAT32_SIZE * 6,
                               FLOAT32_SIZE * 4);
        gl.bindBuffer(gl.ARRAY_BUFFER, pathIDsBuffer);
        gl.vertexAttribPointer(attributes.aPathID, 1, gl.UNSIGNED_SHORT, false, 0, 0);

        gl.enableVertexAttribArray(attributes.aTessCoord);
        gl.enableVertexAttribArray(attributes.aFromPosition);
        gl.enableVertexAttribArray(attributes.aCtrlPosition);
        gl.enableVertexAttribArray(attributes.aToPosition);
        gl.enableVertexAttribArray(attributes.aFromNormal);
        gl.enableVertexAttribArray(attributes.aCtrlNormal);
        gl.enableVertexAttribArray(attributes.aToNormal);
        gl.enableVertexAttribArray(attributes.aPathID);

        renderContext.instancedArraysExt.vertexAttribDivisorANGLE(attributes.aFromPosition, 1);
        renderContext.instancedArraysExt.vertexAttribDivisorANGLE(attributes.aCtrlPosition, 1);
        renderContext.instancedArraysExt.vertexAttribDivisorANGLE(attributes.aToPosition, 1);
        renderContext.instancedArraysExt.vertexAttribDivisorANGLE(attributes.aFromNormal, 1);
        renderContext.instancedArraysExt.vertexAttribDivisorANGLE(attributes.aCtrlNormal, 1);
        renderContext.instancedArraysExt.vertexAttribDivisorANGLE(attributes.aToNormal, 1);
        renderContext.instancedArraysExt.vertexAttribDivisorANGLE(attributes.aPathID, 1);

        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, renderContext.quadElementsBuffer);

        renderContext.vertexArrayObjectExt.bindVertexArrayOES(null);
    }

    private setBlendModeForAA(renderer: Renderer): void {
        const renderContext = renderer.renderContext;
        const gl = renderContext.gl;

        gl.blendEquation(gl.FUNC_ADD);
        gl.blendFunc(gl.ONE, gl.ONE);
        gl.enable(gl.BLEND);
    }
}

/// Switches between mesh-based and stencil-based analytic antialiasing depending on whether stem
/// darkening is enabled.
///
/// FIXME(pcwalton): Share textures and FBOs between the two strategies.
export class AdaptiveStencilMeshAAAStrategy extends AntialiasingStrategy {
    private meshStrategy: MCAAStrategy;
    private stencilStrategy: StencilAAAStrategy;

    get directRenderingMode(): DirectRenderingMode {
        return 'none';
    }

    get passCount(): number {
        return 1;
    }

    constructor(level: number, subpixelAA: SubpixelAAType) {
        super(subpixelAA);
        this.meshStrategy = new MCAAStrategy(level, subpixelAA);
        this.stencilStrategy = new StencilAAAStrategy(level, subpixelAA);
    }

    init(renderer: Renderer): void {
        this.meshStrategy.init(renderer);
        this.stencilStrategy.init(renderer);
    }

    attachMeshes(renderer: Renderer): void {
        this.meshStrategy.attachMeshes(renderer);
        this.stencilStrategy.attachMeshes(renderer);
    }

    setFramebufferSize(renderer: Renderer): void {
        this.meshStrategy.setFramebufferSize(renderer);
        this.stencilStrategy.setFramebufferSize(renderer);
    }

    get transform(): glmatrix.mat4 {
        return this.meshStrategy.transform;
    }

    prepareForRendering(renderer: Renderer): void {
        this.getAppropriateStrategy(renderer).prepareForRendering(renderer);
    }

    prepareForDirectRendering(renderer: Renderer): void {
        this.getAppropriateStrategy(renderer).prepareForDirectRendering(renderer);
    }

    finishAntialiasingObject(renderer: Renderer, objectIndex: number): void {
        this.getAppropriateStrategy(renderer).finishAntialiasingObject(renderer, objectIndex);
    }

    prepareToRenderObject(renderer: Renderer, objectIndex: number): void {
        this.getAppropriateStrategy(renderer).prepareToRenderObject(renderer, objectIndex);
    }

    finishDirectlyRenderingObject(renderer: Renderer, objectIndex: number): void {
        this.getAppropriateStrategy(renderer).finishDirectlyRenderingObject(renderer, objectIndex);
    }

    antialiasObject(renderer: Renderer, objectIndex: number): void {
        this.getAppropriateStrategy(renderer).antialiasObject(renderer, objectIndex);
    }

    resolveAAForObject(renderer: Renderer, objectIndex: number): void {
        this.getAppropriateStrategy(renderer).resolveAAForObject(renderer, objectIndex);
    }

    resolve(pass: number, renderer: Renderer): void {
        this.getAppropriateStrategy(renderer).resolve(pass, renderer);
    }

    worldTransformForPass(renderer: Renderer, pass: number): glmatrix.mat4 {
        return glmatrix.mat4.create();
    }

    private getAppropriateStrategy(renderer: Renderer): AntialiasingStrategy {
        return renderer.needsStencil ? this.stencilStrategy : this.meshStrategy;
    }
}

function calculateStartFromIndexRanges(pathRange: Range, indexRanges: Range[]): number {
    return indexRanges.length === 0 ? 0 : indexRanges[pathRange.start - 1].start;
}

function calculateCountFromIndexRanges(pathRange: Range, indexRanges: Range[]): number {
    if (indexRanges.length === 0)
        return 0;

    let lastIndex;
    if (pathRange.end - 1 >= indexRanges.length)
        lastIndex = unwrapUndef(_.last(indexRanges)).end;
    else
        lastIndex = indexRanges[pathRange.end - 1].start;

    const firstIndex = indexRanges[pathRange.start - 1].start;

    return lastIndex - firstIndex;
}
*/

} // namespace pathfinder

#endif // PATHFINDER_XCAA_STRATEGY_H
