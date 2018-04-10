// pathfinder/src/pathfinder.h
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#ifndef PATHFINDER_IMPL_H
#define PATHFINDER_IMPL_H

#include "text.h"
#include "text-renderer.h"
#include "../include/pathfinder.h"

#include <string>
#include <kraken-math.h>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace pathfinder {

class PathfinderFont;
class ShaderManager;

typedef std::map<GLuint, std::string> ShaderMap;

class TextViewRenderer : public TextRenderer
{
public:
  TextViewRenderer(std::shared_ptr<RenderContext> aRenderContext);
  virtual ~TextViewRenderer();

  kraken::Vector4 getBackgroundColor() const override;
  void prepareToAttachText();
  void layoutText();
  void buildGlyphs();
  void setGlyphTexCoords();
protected:
  kraken::Vector2 getExtraEmboldenAmount() const override;
  void compositeIfNecessary() override;
private:
  GLuint mGlyphPositionsBuffer;
  GLuint mGlyphTexCoordsBuffer;
  GLuint mGlyphElementsBuffer;
  std::vector<float> mGlyphBounds;

  kraken::Vector2 mCameraTranslation;
  kraken::Vector2 mCameraViewSize;
  std::shared_ptr<TextViewImpl> mTextView;
}; // class TextViewRenderer

class TextViewImpl
{
public:
  TextViewImpl();
  virtual ~TextViewImpl();

  bool init();

  void redraw();

  void setText(const std::string& aText);
  std::string getText() const;
  void setFont(std::shared_ptr<Font> aFont);
  std::shared_ptr<PathfinderFont> getFont() const;
  std::shared_ptr<Font> getPublicFont() const;
  void setFontSize(float aFontSize);
  float getFontSize() const;
  void setEmboldenAmount(float aEmboldenAmount);
  float getEmboldenAmount() const;
  void setRotationAngle(float aRotationAngle);
  float getRotationAngle() const;
  std::shared_ptr<GlyphStore> getGlyphStore();
  bool getUseHinting() const;
  std::vector<AtlasGlyph> getAtlasGlyphs();
  void setAtlasGlyphs(const std::vector<AtlasGlyph>& aAtlasGlyphs);
  std::shared_ptr<Atlas> getAtlas();
  SimpleTextLayout& getLayout();

private:
  std::string mText;
  float mFontSize;
  float mEmboldenAmount;
  float mRotationAngle;
  bool mDirtyConfig;

  std::shared_ptr<TextViewRenderer> mRenderer;
  std::shared_ptr<Font> mFont;
  std::shared_ptr<SimpleTextLayout> mLayout;
  std::shared_ptr<GlyphStore> mGlyphStore;
  std::shared_ptr<PathfinderPackedMeshes> mMeshes;

  std::vector<AtlasGlyph> mAtlasGlyphs;
  std::shared_ptr<Atlas> mAtlas;
 
  void recreateLayout();

}; // class TextViewImpl

class FontImpl
{
public:
  FontImpl();
  ~FontImpl();
  bool load(const unsigned char* aData, size_t aDataLength);
  std::shared_ptr<PathfinderFont> getFont();
private:
  std::shared_ptr<PathfinderFont> mFont;
  FT_Library mFTLibrary;
}; // class FontImpl

} // namespace pathfinder

#endif // PATHFINDER_TEXT_VIEW_IMPL_H

