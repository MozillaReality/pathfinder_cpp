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
#include "context.h"

#include <vector>
#include <kraken-math.h>

using namespace std;
using namespace kraken;

namespace pathfinder {

class Font;
class GlyphStore;
class PathfinderShaderProgram;

class TextRenderer : public Renderer
{
public:
  TextRenderer(std::shared_ptr<RenderContext> aRenderContext);
  GLuint mAtlasFramebuffer;
  GLuint mAtlasDepthTexture;
  GLuint mGlyphPositionsBuffer;
  GLuint mGlyphTexCoordsBuffer;
  GLuint mGlyphElementsBuffer;
  void setText(const std::string& aText);
  std::string getText() const;
  bool getIsMulticolor() const override;
  bool getNeedsStencil() const override;
  GLuint getDestFramebuffer() const override;
  kraken::Vector2i getDestAllocatedSize() const override;
  kraken::Vector2i getDestUsedSize() const override;
  kraken::Vector2 getTotalEmboldenAmount() const override;
  void setEmboldenAmount(float aEmboldenAmount);
  float getEmboldenAmount() const;
  kraken::Vector4 getBGColor() const override;
  kraken::Vector4 getFGColor() const override;
  float getRotationAngle() const;
  void setRotationAngle(float aRotationAngle);
  float getPixelsPerUnit() const;
  kraken::Matrix4 getWorldTransform() const override;
  kraken::Vector2 getStemDarkeningAmount() const;
  kraken::Vector2 getUsedSizeFactor() const override;
  void setHintsUniform(PathfinderShaderProgram& aProgram) override;
  float* pathBoundingRects(int objectIndex) override;
  virtual int pathBoundingRectsLength(int objectIndex) override;


  std::vector<AtlasGlyph> getAtlasGlyphs();
  void setAtlasGlyphs(const std::vector<AtlasGlyph>& aAtlasGlyphs);
  std::shared_ptr<Atlas> getAtlas();
  std::shared_ptr<GlyphStore> getGlyphStore();
  std::shared_ptr<PathfinderFont> getFont() const;
  void setFont(std::shared_ptr<PathfinderFont> aFont);
  float getFontSize() const;
  void setFontSize(float aFontSize);
  bool getUseHinting() const;
  void setUseHinting(bool aUseHinting);

  void layout();
  void prepareToAttachText();
  void buildGlyphs(Vector2 aViewTranslation, Vector2 aViewSize);

protected:

  void recreateLayout();
  void layoutText();

  void compositeIfNecessary(Vector2 aViewTranslation, Vector2 aViewSize) override;
  void setGlyphTexCoords();

  kraken::Vector2 getExtraEmboldenAmount() const;
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
  ProgramID getDirectCurveProgramName() override;
  ProgramID getDirectInteriorProgramName(DirectRenderingMode renderingMode) override;
private:
  StemDarkeningMode mStemDarkening;
  SubpixelAAType mSubpixelAA;
  std::vector<AtlasGlyph> mAtlasGlyphs;
  std::shared_ptr<PathfinderFont> mFont;
  std::shared_ptr<GlyphStore> mGlyphStore;
  std::shared_ptr<SimpleTextLayout> mLayout;
  std::shared_ptr<Atlas> mAtlas;
  std::shared_ptr<PathfinderPackedMeshes> mMeshes;
  std::vector<float> mGlyphBounds;
  std::string mText;
  float mFontSize;
  float mExtraEmboldenAmount;
  bool mUseHinting;
  bool mRotationAngle;
  bool mDirtyConfig;

  int getPathCount();
  int getObjectCount() const override;

}; // class TextRenderer

} // namespace pathfinder

#endif // PATHFINDER_TEXT_RENDER_H
