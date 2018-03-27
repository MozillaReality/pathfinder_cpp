// pathfinder/src/atlas.cpp
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include "atlas.h"

using namespace std;
using namespace kraken;

namespace pathfinder {

Atlas::Atlas()
  : mTexture(0)
{
  mUsedSize = kraken::Vector2i::Zero();
}

void
Atlas::layoutGlyphs(std::vector<AtlasGlyph>& glyphs,
                  PathfinderFont& font,
                  int pixelsPerUnit,
                  int rotationAngle,
                  Hint hint,
                  kraken::Vector2 emboldenAmount)
{
  Vector2 nextOrigin = Vector2::One();
  float shelfBottom = 2.0f;

  for (AtlasGlyph& glyph: glyphs) {
    // Place the glyph, and advance the origin.
    const metrics = font.metricsForGlyph(glyph.glyphKey.id);
    if (metrics == null) {
      continue;
    }

    UnitMetrics unitMetrics(metrics, rotationAngle, emboldenAmount);
    glyph.setPixelLowerLeft(nextOrigin, unitMetrics, pixelsPerUnit);

    Vector2 pixelOrigin = glyph.calculateSubpixelOrigin(pixelsPerUnit);
    nextOrigin[0] = calculatePixelRectForGlyph(unitMetrics,
                                               pixelOrigin,
                                               pixelsPerUnit,
                                               hint)[2] + 1.0f;

    // If the glyph overflowed the shelf, make a new one and reposition the glyph.
    if (nextOrigin[0] > ATLAS_SIZE[0]) {
        nextOrigin = Vector2::Create((1.0f, shelfBottom + 1.0f);
        glyph.setPixelLowerLeft(nextOrigin, unitMetrics, pixelsPerUnit);
        pixelOrigin = glyph.calculateSubpixelOrigin(pixelsPerUnit);
        nextOrigin[0] = calculatePixelRectForGlyph(unitMetrics,
                                                   pixelOrigin,
                                                   pixelsPerUnit,
                                                   hint)[2] + 1.0f;
    }

    // Grow the shelf as necessary.
    float glyphBottom = calculatePixelRectForGlyph(unitMetrics,
                                                   pixelOrigin,
                                                   pixelsPerUnit,
                                                   hint)[3];
    shelfBottom = Math.max(shelfBottom, glyphBottom + 1.0f);
  }

  // FIXME(pcwalton): Could be more precise if we don't have a full row.
  mUsedSize = Vector2::Create(ATLAS_SIZE[0], shelfBottom);
}

GLuint Atlas::ensureTexture(RenderContext& renderContext)
  if (mTexture != 0) {
    return mTexture;
  }

  glCreateTextures(GL_TEXTURE_2D, 1, &mTexture);
  glBindTexture(GL_TEXTURE_2D, mTexture);
  glTexImage2D(GL_TEXTURE_2D,
                0,
                GL_RGBA,
                ATLAS_SIZE[0],
                ATLAS_SIZE[1],
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                0);
  setTextureParameters(GL_NEAREST);

  return mTexture;
}

kraken::Vector2i
Atlas::getUsedSize() const
{
  return mUsedSize;
}

AtlasGlyph::AtlasGlyph(int aGlyphStoreIndex, GlyphKey glyphKey)
 : mGlyphStoreIndex(aGlyphStoreIndex)
 , mGlyphKey(glyphKey)
 , mOrigin(Vector2::Zero())
{

}

int
AtlasGlyph::getGlyphStoreIndex()
{
  return mGlyphStoreIndex;
}
GlyphKey
AtlasGlyph::getGlyphKey()
{
  return mGlyphKey;
}
Vector2
AtlasGlyph::getOrigin()
{
  return mOrigin;
}

kraken::Vector2
AtlasGlyph::calculateSubpixelOrigin(float pixelsPerUnit)
{
  Vector2 pixelOrigin = mOrigin * pixelsPerUnit;
  pixelOrigin = Vector2::Create(round(pixelOrigin.x), round(pixelOrigin.y);
  if (mGlyphKey.getHasSubpixel) {
    pixelOrigin[0] += mGlyphKey.subpixel / SUBPIXEL_GRANULARITY;
  }
  return pixelOrigin;
}

void
AtlasGlyph::setPixelLowerLeft(kraken::Vector2 pixelLowerLeft, UnitMetrics& metrics, float pixelsPerUnit)
{
  float pixelXMin = calculatePixelXMin(metrics, pixelsPerUnit);
  float pixelDescent = calculatePixelDescent(metrics, pixelsPerUnit);
  Vector2 pixelOrigin = Vector2::Create(pixelLowerLeft[0] - pixelXMin,
                                        pixelLowerLeft[1] - pixelDescent);
  setPixelOrigin(pixelOrigin, pixelsPerUnit);
}

void
AtlasGlyph::setPixelOrigin(kraken::Vector2 pixelOrigin, float pixelsPerUnit)
{
  mOrigin = pixelOrigin / pixelsPerUnit;
}


int
AtlasGlyph::getPathId()
{
  if (!mGlyphKey.getHasSubpixel) {
    return mGlyphStoreIndex + 1;
  }
  return mGlyphStoreIndex * SUBPIXEL_GRANULARITY + mGlyphKey.subpixel + 1;
}

AtlasGlyph::GlyphKey(int aID, bool aHasSubpixel, float aSubpixel)
  : mID(aID)
  , mSubpixel(aSubpixel)
  , mHasSubpixel(aHasSubpixel)
{

}

int
AtlasGlyph::getID()
{
  return mID;
}

float
AtlasGlyph::getSubpixel()
{
  return mSubpixel;
}

bool
AtlasGlyph::getHasSubpixel()
{
  return mHasSubpixel;
}

int
AtlasGlyph::getSortKey()
{
  return mHasSubpixel ? mID * SUBPIXEL_GRANULARITY + mSubpixel : mID;
}

} // namespace pathfinder
