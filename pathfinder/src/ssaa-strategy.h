#ifndef PATHFINDER_SSAA_STRATEGY_H
#define PATHFINDER_SSAA_STRATEGY_H

#include "aa-strategy.h"
#include "platform.h"

#include <kraken-math.h>

namespace pathfinder {

class SSAAStrategy : public AntialiasingStrategy
{
public:
  SSAAStrategy(int aLevel, SubpixelAAType aSubpixelAA);
  int getPassCount() const override;
  virtual void attachMeshes(RenderContext& renderContext, Renderer& renderer) override { }
  virtual void setFramebufferSize(Renderer& renderer) override;
  virtual kraken::Matrix4 getTransform() const override;
  virtual void prepareForRendering(Renderer& renderer) override;
  virtual void prepareForDirectRendering(Renderer& renderer) override { }
  virtual void prepareToRenderObject(Renderer& renderer, int objectIndex) override;
  virtual void finishDirectlyRenderingObject(Renderer& renderer, int objectIndex) override { }
  virtual void antialiasObject(Renderer& renderer, int objectIndex) override { }
  virtual void finishAntialiasingObject(Renderer& renderer, int objectIndex) override { }
  virtual void resolveAAForObject(Renderer& renderer, int objectIndex) override { }
  virtual void resolve(int pass, Renderer& renderer) override;
  virtual kraken::Matrix4 getWorldTransformForPass(Renderer& renderer, int pass) override;
  DirectRenderingMode getDirectRenderingMode() const override;
protected:
private:
  int mLevel;
  kraken::Vector2i supersampledFramebufferSize;
  kraken::Vector2i destFramebufferSize;
  GLuint supersampledColorTexture;
  GLuint supersampledDepthTexture;
  GLuint supersampledFramebuffer;

  TileInfo tileInfoForPass(int pass);
  kraken::Vector2i supersampleScale() const;
  kraken::Vector2i getTileSize() const;
  kraken::Vector2i usedSupersampledFramebufferSize(Renderer& renderer) const;

}; // class SSAAStrategy

} // namespace pathfinder

#endif // PATHFINDER_SSAA_STRATEGY_H
