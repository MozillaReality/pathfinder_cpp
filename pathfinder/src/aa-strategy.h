// pathfinder/src/aa-strategy.h
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#ifndef PATHFINDER_AA_STRATEGY_H
#define PATHFINDER_AA_STRATEGY_H

#include <hydra.h>
#include "shader-loader.h"
#include "context.h"

namespace pathfinder {

class Renderer;

typedef enum
{
  saat_none = 0,
  saat_freetype,
  saat_core_graphics
} SubpixelAAType;

const float SUBPIXEL_AA_KERNELS[][4] = {
  // These intentionally do not precisely match what Core Graphics does (a Lanczos function),
  // because we don't want any ringing artefacts.
  {0.0f, 0.0f, 0.0f, 1.0f}, // saat_none
  {0.0f, 0.031372549f, 0.301960784f, 0.337254902f}, // saat_freetype
  {0.033165660f, 0.102074051f, 0.221434336f, 0.286651906f} // saat_core_graphics
};

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
  kraken::Vector2i size;
  kraken::Vector2i position;
};

class AntialiasingStrategy {
public:
  AntialiasingStrategy(SubpixelAAType aSubpixelAA)
    : mSubpixelAA(aSubpixelAA)
  {

  }
  virtual ~AntialiasingStrategy()
  {

  }
  AntialiasingStrategy(const AntialiasingStrategy&) = delete;
  AntialiasingStrategy& operator=(const AntialiasingStrategy&) = delete;

  // The type of direct rendering that should occur, if any.
  virtual DirectRenderingMode getDirectRenderingMode() const = 0;

  // How many rendering passes this AA strategy requires.
  virtual int getPassCount() const = 0;

  // Prepares any OpenGL data. This is only called on startup and canvas resize.
  virtual bool init(Renderer& renderer);

  // Uploads any mesh data. This is called whenever a new set of meshes is supplied.
  virtual void attachMeshes(RenderContext& renderContext, Renderer& renderer) = 0;

  // This is called whenever the framebuffer has changed.
  virtual void setFramebufferSize(Renderer& renderer)
  {
  }

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

  void setSubpixelAAKernelUniform(Renderer& renderer, PathfinderShaderProgram& aProgram);

  virtual kraken::Matrix4 getWorldTransformForPass(Renderer& renderer, int pass);

protected:
  SubpixelAAType mSubpixelAA;
}; // class AntialiasingStrategy

class NoAAStrategy : public AntialiasingStrategy
{
public:
  NoAAStrategy(int level, SubpixelAAType subpixelAA)
    : AntialiasingStrategy(subpixelAA)
  {
    mFramebufferSize.init();
  }

  int getPassCount() const override {
    return 1;
  }

  void attachMeshes(RenderContext& renderContext, Renderer& renderer) override { };

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
  kraken::Vector2i mFramebufferSize;
}; // class NoAAStrategy

} // namepsace pathfinder

#endif // PATHFINDER_AA_STRATEGY_H
