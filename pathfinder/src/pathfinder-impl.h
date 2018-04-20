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

class TextViewImpl
{
public:
  TextViewImpl();
  virtual ~TextViewImpl();
  TextViewImpl(const TextViewImpl&) = delete;
  TextViewImpl& operator=(const TextViewImpl&) = delete;

  bool init();

  void redraw();
  void layout();

  void setText(const std::string& aText);
  std::string getText() const;
  void setFont(std::shared_ptr<Font> aFont);
  std::shared_ptr<Font> getFont() const;
  void setFontSize(float aFontSize);
  float getFontSize() const;
  void setEmboldenAmount(float aEmboldenAmount);
  float getEmboldenAmount() const;
  void setRotationAngle(float aRotationAngle);
  float getRotationAngle() const;
  bool getUseHinting() const;
  void setUseHinting(bool aUseHinting);
  std::shared_ptr<Atlas> getAtlas();

private:
  kraken::Vector2 mCameraTranslation;
  kraken::Vector2 mCameraViewSize;

  std::shared_ptr<TextRenderer> mRenderer;
  std::shared_ptr<RenderContext> mRenderContext;
  std::shared_ptr<Font> mFont;

}; // class TextViewImpl

class FontImpl
{
public:
  FontImpl();
  ~FontImpl();
  FontImpl(const PathfinderPackedMeshes&) = delete;
  FontImpl& operator=(const PathfinderPackedMeshes&) = delete;
  bool load(const unsigned char* aData, size_t aDataLength);
  std::shared_ptr<PathfinderFont> getFont();
private:
  std::shared_ptr<PathfinderFont> mFont;
  FT_Library mFTLibrary;
}; // class FontImpl

} // namespace pathfinder

#endif // PATHFINDER_TEXT_VIEW_IMPL_H

