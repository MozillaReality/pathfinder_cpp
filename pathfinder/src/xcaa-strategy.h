#ifndef PATHFINDER_XCAA_STRATEGY_H
#define PATHFINDER_XCAA_STRATEGY_H

#include "aa-strategy.h"
#include "platform.h"
#include "buffer-texture.h"
#include "shader-loader.h"
#include "render-context.h"
#include "utils.h"

#include <vector>
#include <map>
#include <memory>
#include <kraken-math.h>

namespace pathfinder {

struct FastEdgeVAOs {
  GLuint upper;
  GLuint lower;
};

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
  virtual void prepareAA(Renderer& renderer);
  void setAAState(Renderer& renderer);
  virtual void setAAUniforms(Renderer& renderer, UniformMap& uniforms, int objectIndex);
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

class MCAAStrategy : public XCAAStrategy
{
public:
  MCAAStrategy(int aLevel, SubpixelAAType aSubpixelAA)
    : XCAAStrategy(aLevel, aSubpixelAA)
    , mVAO(0)
  { }
  virtual void attachMeshes(RenderContext& renderContext, Renderer& renderer) override;
  virtual void antialiasObject(Renderer& renderer, int objectIndex) override;
  virtual DirectRenderingMode getDirectRenderingMode() const override;
protected:
  TransformType getTransformType() const override;
  bool getMightUseAAFramebuffer() const override;
  bool usesAAFramebuffer(Renderer& renderer) override;
  bool usesResolveProgram(Renderer& renderer) override;
  virtual PathfinderShaderProgram& getResolveProgram(Renderer& renderer) override;
  virtual void clearForAA(Renderer& renderer) override;
  virtual void setAADepthState(Renderer& renderer) override;
  virtual void clearForResolve(Renderer& renderer) override;
  
  void prepareAA(Renderer& renderer) override;
  void initVAOForObject(Renderer& renderer, int objectIndex);
  PathfinderShaderProgram& edgeProgram(Renderer& renderer);
  void antialiasEdgesOfObjectWithProgram(Renderer& renderer,
                                         int objectIndex,
                                         PathfinderShaderProgram& shaderProgram);
  void setAAUniforms(Renderer& renderer, UniformMap& uniforms, int objectIndex) override;
private:
  GLuint mVAO;
  void setBlendModeForAA(Renderer& renderer);

}; // class MCAAStrategy;

class StencilAAAStrategy : public XCAAStrategy
{
public:
  StencilAAAStrategy(int aLevel, SubpixelAAType aSubpixelAA)
    : XCAAStrategy(aLevel, aSubpixelAA)
    , mVAO(0)
  { }
  virtual DirectRenderingMode getDirectRenderingMode() const override;
  virtual void attachMeshes(RenderContext& renderContext, Renderer& renderer) override;
  virtual void antialiasObject(Renderer& renderer, int objectIndex) override;
  bool usesResolveProgram(Renderer& renderer) override;
protected:
  TransformType getTransformType() const override;
  bool getMightUseAAFramebuffer() const override;
  bool usesAAFramebuffer(Renderer& renderer) override;
  virtual void clearForAA(Renderer& renderer) override;
  virtual PathfinderShaderProgram& getResolveProgram(Renderer& renderer) override;
  virtual void setAADepthState(Renderer& renderer) override;
  virtual void setAAUniforms(Renderer& renderer, UniformMap& uniforms, int objectIndex);
  virtual void clearForResolve(Renderer& renderer) override;
private:
  void createVAO(Renderer& renderer);
  void setBlendModeForAA(Renderer& renderer);
  GLuint mVAO;
};

/// Switches between mesh-based and stencil-based analytic antialiasing depending on whether stem
/// darkening is enabled.
///
/// FIXME(pcwalton): Share textures and FBOs between the two strategies.
/*
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




*/

int calculateStartFromIndexRanges(Range pathRange, std::vector<Range>& indexRanges);
int calculateCountFromIndexRanges(Range pathRange, std::vector<Range>& indexRanges);

} // namespace pathfinder

#endif // PATHFINDER_XCAA_STRATEGY_H
