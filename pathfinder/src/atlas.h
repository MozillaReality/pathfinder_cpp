// pathfinder/src/atlas.h
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#ifndef PATHFINDER_ATLAS_H
#define PATHFINDER_ATLAS_H

#include "platform.h"
#include <kraken-math.h>
#include <vector>

namespace pathfinder {

class AtlasGlyph;
class PathfinderFont;
class Hint;
class RenderContext;
class UnitMetrics;

const int SUBPIXEL_GRANULARITY = 4;
const kraken::Vector2i ATLAS_SIZE = kraken::Vector2i::Create(2048, 4096);

class Atlas
{
public:
  Atlas();
  void layoutGlyphs(std::vector<AtlasGlyph>& glyphs,
                    PathfinderFont& font,
                    float pixelsPerUnit,
                    float rotationAngle,
                    const Hint& hint,
                    kraken::Vector2 emboldenAmount);
  GLuint ensureTexture(RenderContext& renderContext);
  kraken::Vector2i getUsedSize() const;
private:
  GLuint mTexture;
  kraken::Vector2i mUsedSize;
}; // class Atlas

class GlyphKey
{
public:
  GlyphKey(int aID, bool aHasSubpixel, float aSubpixel);
  int getID() const;
  float getSubpixel() const;
  bool getHasSubpixel() const;
  int getSortKey() const;
private:
  int mID;
  float mSubpixel;
  bool mHasSubpixel;
}; // class GlyphKey

class AtlasGlyph
{
public:
  AtlasGlyph(int aGlyphStoreIndex, GlyphKey glyphKey);
  int getGlyphStoreIndex();
  const GlyphKey getGlyphKey() const;
  kraken::Vector2 getOrigin();
  kraken::Vector2 calculateSubpixelOrigin(float pixelsPerUnit) const;
  void setPixelLowerLeft(kraken::Vector2 pixelLowerLeft, UnitMetrics& metrics, float pixelsPerUnit);
  int getPathID() const;

private:
  int mGlyphStoreIndex;
  GlyphKey mGlyphKey;
  kraken::Vector2 mOrigin;

  void setPixelOrigin(kraken::Vector2 pixelOrigin, float pixelsPerUnit);
}; // class AtlasGlyph


} // namespace pathfinder

#endif // PATHFINDER_ATLAS_H
