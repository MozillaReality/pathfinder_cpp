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
  void prepareForRendering(Renderer& renderer) override { }
  void prepareForDirectRendering(Renderer& renderer) override { }
  void prepareToRenderObject(Renderer& renderer, int objectIndex) override { }
 
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
  virtual void setAAUniforms(Renderer& renderer, UniformMap& uniforms, int objectIndex) override;
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
class AdaptiveStencilMeshAAAStrategy : public AntialiasingStrategy
{
public:
  AdaptiveStencilMeshAAAStrategy(int level, SubpixelAAType subpixelAA);
  virtual DirectRenderingMode getDirectRenderingMode() const override;
  int getPassCount() const override;
  virtual void init(Renderer& renderer) override;
  virtual void attachMeshes(RenderContext& renderContext, Renderer& renderer) override;
  virtual void setFramebufferSize(Renderer& renderer) override;
  virtual kraken::Matrix4 getTransform() const override;
  virtual void prepareForRendering(Renderer& renderer) override;
  virtual void prepareForDirectRendering(Renderer& renderer) override;
  virtual void finishAntialiasingObject(Renderer& renderer, int objectIndex) override;
  void prepareToRenderObject(Renderer& renderer, int objectIndex) override;
  void finishDirectlyRenderingObject(Renderer& renderer, int objectIndex) override;
  void antialiasObject(Renderer& renderer, int objectIndex) override;
  void resolveAAForObject(Renderer& renderer, int objectIndex) override;
  void resolve(int pass, Renderer& renderer) override;
  kraken::Matrix4 getWorldTransformForPass(Renderer& renderer, int pass) override;

protected:
private:
  std::unique_ptr<MCAAStrategy> mMeshStrategy;
  std::unique_ptr<StencilAAAStrategy> mStencilStrategy;

  AntialiasingStrategy& getAppropriateStrategy(Renderer& renderer);

}; // class AdaptiveStencilMeshAAAStrategy

int calculateStartFromIndexRanges(Range pathRange, std::vector<Range>& indexRanges);
int calculateCountFromIndexRanges(Range pathRange, std::vector<Range>& indexRanges);

} // namespace pathfinder

#endif // PATHFINDER_XCAA_STRATEGY_H
