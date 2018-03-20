#ifndef PATHFINDER_AA_STRATEGY_H
#define PATHFINDER_AA_STRATEGY_H

#include <kraken-math.h>

namespace pathfinder {

class Renderer;

typedef enum
{
  asn_none,
  asn_ssaa,
  asn_xcaa
} AntialiasingStrategyName;

typedef enum
{
  drm_none,
  drm_conservative,
  drm_color
} DirectRenderingMode;

typedef enum
{
  spaa_none,
  spaa_medium
} SubpixelAAType;

typedef enum
{
  gcm_off,
  gcm_on
} GammaCorrectionMode;

typedef enum
{
  sdm_none,
  sdm_dark
} StemDarkeningMode;

typedef struct {
  GammaCorrectionMode gammaCorrection;
  StemDarkeningMode stemDarkening;
  SubpixelAAType subpixelAA;
} AAOptions;


struct TileInfo
{
  kraken::Vector2 size;
  kraken::Vector2 position;
};

class AntialiasingStrategy {
public:
  // The type of direct rendering that should occur, if any.
  virtual DirectRenderingMode getDirectRenderingMode() const = 0;

  // How many rendering passes this AA strategy requires.
  virtual int getPassCount() const = 0;

  // Prepares any OpenGL data. This is only called on startup and canvas resize.
  void init(Renderer& renderer);

  // Uploads any mesh data. This is called whenever a new set of meshes is supplied.
  virtual void attachMeshes(Renderer& renderer) = 0;

  // This is called whenever the framebuffer has changed.
  virtual void setFramebufferSize(Renderer& renderer) = 0;

  // Returns the transformation matrix that should be applied when directly rendering.
  virtual kraken::Matrix4 getTransform() const = 0;

  // Called before rendering.
  //
  // Typically, this redirects rendering to a framebuffer of some sort.
  virtual void prepareForRendering(Renderer& renderer) = 0;

  // Called before directly rendering.
  //
  // Typically, this redirects rendering to a framebuffer of some sort.
  virtual void prepareForDirectRendering(Renderer& renderer) = 0;

  // Called before directly rendering a single object.
  virtual void prepareToRenderObject(Renderer& renderer, int objectIndex) = 0;

  virtual void finishDirectlyRenderingObject(Renderer& renderer, int objectIndex) = 0;

  // Called after direct rendering.
  //
  // This usually performs the actual antialiasing.
  virtual void antialiasObject(Renderer& renderer, int objectIndex) = 0;

  // Called after antialiasing each object.
  virtual void finishAntialiasingObject(Renderer& renderer, int objectIndex) = 0;

  // Called before rendering each object directly.
  virtual void resolveAAForObject(Renderer& renderer, int objectIndex) = 0;

  // Called after antialiasing.
  //
  // This usually blits to the real framebuffer.
  virtual void resolve(int pass, Renderer& renderer) = 0;

  kraken::Matrix4 getWorldTransformForPass(Renderer& renderer, int pass);
}; // class AntialiasingStrategy

class NoAAStrategy : public AntialiasingStrategy
{
  NoAAStrategy(int level, NoAAStrategy subpixelAA) {
    mFramebufferSize.init();
  }

  int getPassCount() const override {
    return 1;
  }

  void attachMeshes(Renderer& renderer) override { };

  void setFramebufferSize(Renderer& renderer) override;

  kraken::Matrix4 getTransform() const override {
    return kraken::Matrix4::Identity();
  }

  void prepareForRendering(Renderer& renderer) override;

  void prepareForDirectRendering(Renderer& renderer) override { }

  void prepareToRenderObject(Renderer& renderer, int objectIndex) override;

  void finishDirectlyRenderingObject(Renderer& renderer, int objectIndex) override { }

  void antialiasObject(Renderer& renderer, int objectIndex) override { }

  void finishAntialiasingObject(Renderer& renderer, int objectIndex) override { }

  void resolveAAForObject(Renderer& renderer, int objectIndex) override { }

  void resolve(int pass, Renderer& renderer) override { }

  DirectRenderingMode getDirectRenderingMode() const override {
    return drm_color;
  }

private:
  kraken::Vector2 mFramebufferSize;
}; // class NoAAStrategy

} // namepsace pathfinder

#endif // PATHFINDER_AA_STRATEGY_H