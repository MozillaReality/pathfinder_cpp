// pathfinder/src/text-renderer.h
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#ifndef PATHFINDER_TEXT_RENDER_H
#define PATHFINDER_TEXT_RENDER_H

#include "platform.h"
#include "atlas.h"
#include "text.h"
#include "renderer.h"
#include "render-context.h"

#include <vector>
#include <kraken-math.h>

using namespace std;
using namespace kraken;

namespace pathfinder {

class TextRenderContext : public RenderContext
{
public:

  virtual std::vector<AtlasGlyph> getAtlasGlyphs() = 0;
  virtual void setAtlasGlyphs(const std::vector<AtlasGlyph>& aAtlasGlyphs) = 0;
  virtual std::shared_ptr<Atlas> getAtlas() = 0;
  virtual std::shared_ptr<GlyphStore> getGlyphStore() = 0;
  virtual std::shared_ptr<PathfinderFont> getFont() const = 0;
  virtual float getFontSize() const = 0;
  virtual bool getUseHinting() const = 0;

}; // class TextRenderContext

class TextRenderer : public Renderer
{
public:
  TextRenderer(std::shared_ptr<TextRenderContext> aRenderContext);
  std::shared_ptr<TextRenderContext> mRenderContext;
  // todo(kearwood) - Implement OrthographicCamera and uncomment:
  // std::shared_ptr<OrthographicCamera> mCamera; 
  GLuint mAtlasFramebuffer;
  GLuint mAtlasDepthTexture;
  bool getIsMulticolor() const override;
  bool getNeedsStencil() const override;
  GLuint getDestFramebuffer() const override;
  kraken::Vector2i getDestAllocatedSize() const override;
  kraken::Vector2i getDestUsedSize() const override;
  kraken::Vector2 getEmboldenAmount() const override;
  kraken::Vector4 getBGColor() const override;
  kraken::Vector4 getFGColor() const override;
  float getRotationAngle() const;
  float getPixelsPerUnit() const;
  kraken::Matrix4 getWorldTransform() const override;
  kraken::Vector2 getStemDarkeningAmount() const;
  kraken::Vector2 getUsedSizeFactor() const override;
  void setHintsUniform(UniformMap& uniforms) override;
  float* pathBoundingRects(int objectIndex) override;
  virtual int pathBoundingRectsLength(int objectIndex) override;
protected:
  virtual kraken::Vector2 getExtraEmboldenAmount() const;
  void createAtlasFramebuffer();
  std::shared_ptr<AntialiasingStrategy> createAAStrategy(AntialiasingStrategyName aaType,
                                        int aaLevel,
                                        SubpixelAAType subpixelAA,
                                        StemDarkeningMode stemDarkening) override;
  void clearForDirectRendering() { }
  void buildAtlasGlyphs(std::vector<AtlasGlyph> aAtlasGlyphs);
  std::vector<__uint8_t> pathColorsForObject(int objectIndex) override;
  std::shared_ptr<PathTransformBuffers<std::vector<float>>> pathTransformsForObject(int objectIndex) override;
  std::shared_ptr<Hint> createHint();
  ShaderID getDirectCurveProgramName() override;
  ShaderID getDirectInteriorProgramName(DirectRenderingMode renderingMode) override;
private:
  StemDarkeningMode mStemDarkening;
  SubpixelAAType mSubpixelAA;

  int getPathCount();
  int getObjectCount() const override;
}; // class TextRenderer

} // namespace pathfinder

#endif // PATHFINDER_TEXT_RENDER_H
