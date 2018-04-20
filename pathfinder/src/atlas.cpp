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
#include "text.h"
#include "gl-utils.h"

#include <algorithm>
#include <assert.h>

using namespace std;
using namespace kraken;

namespace pathfinder {

Atlas::Atlas()
  : mTexture(0)
{
  mUsedSize = kraken::Vector2i::Zero();
}

Atlas::~Atlas()
{
  if (mTexture) {
    GLDEBUG(glDeleteTextures(1, &mTexture));
    mTexture = 0;
  }
}


bool
Atlas::init(RenderContext& renderContext)
{
  GLDEBUG(glCreateTextures(GL_TEXTURE_2D, 1, &mTexture));
  GLDEBUG(glBindTexture(GL_TEXTURE_2D, mTexture));
  GLDEBUG(glTexImage2D(GL_TEXTURE_2D,
                0,
                GL_RGBA,
                ATLAS_SIZE[0],
                ATLAS_SIZE[1],
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                0));
  setTextureParameters(GL_NEAREST);

  return true;
}

void
Atlas::layoutGlyphs(std::vector<AtlasGlyph>& glyphs,
                  PathfinderFont& font,
                  float pixelsPerUnit,
                  float rotationAngle,
                  const Hint& hint,
                  kraken::Vector2 emboldenAmount)
{
  Vector2 nextOrigin = Vector2::One();
  float shelfBottom = 2.0f;

  for (AtlasGlyph& glyph: glyphs) {
    // Place the glyph, and advance the origin.
    FT_BBox metrics = font.metricsForGlyph(glyph.getGlyphKey().getID());

    UnitMetrics unitMetrics(metrics, rotationAngle, emboldenAmount);
    glyph.setPixelLowerLeft(nextOrigin, unitMetrics, pixelsPerUnit);

    Vector2 pixelOrigin = glyph.calculateSubpixelOrigin(pixelsPerUnit);
    Vector4 pixelRect = calculatePixelRectForGlyph(unitMetrics,
                          pixelOrigin,
                          pixelsPerUnit,
                          hint);
    nextOrigin[0] = pixelRect[2] + 1.0f;

    // If the glyph overflowed the shelf, make a new one and reposition the glyph.
    if (nextOrigin[0] > ATLAS_SIZE[0]) {
        nextOrigin = Vector2::Create(1.0f, shelfBottom + 1.0f);
        glyph.setPixelLowerLeft(nextOrigin, unitMetrics, pixelsPerUnit);
        pixelOrigin = glyph.calculateSubpixelOrigin(pixelsPerUnit);
        pixelRect = calculatePixelRectForGlyph(unitMetrics,
                                               pixelOrigin,
                                               pixelsPerUnit,
                                               hint);
        nextOrigin[0] = pixelRect[2] + 1.0f;
    }

    // Grow the shelf as necessary.
    float glyphBottom = pixelRect[3];
    shelfBottom = max(shelfBottom, glyphBottom + 1.0f);
  }

  // FIXME(pcwalton): Could be more precise if we don't have a full row.
  mUsedSize = Vector2i::Create(ATLAS_SIZE[0], shelfBottom);
}

GLuint
Atlas::getTexture()
{
  assert(mTexture != 0);
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
const GlyphKey
AtlasGlyph::getGlyphKey() const
{
  return mGlyphKey;
}
Vector2
AtlasGlyph::getOrigin()
{
  return mOrigin;
}

kraken::Vector2
AtlasGlyph::calculateSubpixelOrigin(float pixelsPerUnit) const
{
  Vector2 pixelOrigin = mOrigin * pixelsPerUnit;
  pixelOrigin = Vector2::Create(round(pixelOrigin.x), round(pixelOrigin.y));
  if (mGlyphKey.getSubpixel() != -1) {
    pixelOrigin[0] += mGlyphKey.getSubpixel() / SUBPIXEL_GRANULARITY;
  }
  return pixelOrigin;
}

void
AtlasGlyph::setPixelLowerLeft(kraken::Vector2 pixelLowerLeft, UnitMetrics& metrics, float pixelsPerUnit)
{
  float pixelXMin = floorf(metrics.mLeft * pixelsPerUnit);
  float pixelDescent = floorf(metrics.mDescent * pixelsPerUnit);
  Vector2 pixelOrigin = Vector2::Create(pixelLowerLeft.x - pixelXMin,
                                        pixelLowerLeft.y - pixelDescent);
  mOrigin = pixelOrigin / pixelsPerUnit;
}


int
AtlasGlyph::getPathID() const
{
  if (mGlyphKey.getSubpixel() == -1) {
    return mGlyphStoreIndex + 1;
  }
  return mGlyphStoreIndex * SUBPIXEL_GRANULARITY + mGlyphKey.getSubpixel() + 1;
}

GlyphKey::GlyphKey(int aID, int aSubpixel)
  : mID(aID)
  , mSubpixel(aSubpixel)
{

}

int
GlyphKey::getID() const
{
  return mID;
}

int
GlyphKey::getSubpixel() const
{
  return mSubpixel;
}

int
GlyphKey::getSortKey() const
{
  return mSubpixel == -1 ? mID : mID * SUBPIXEL_GRANULARITY + mSubpixel;
}

} // namespace pathfinder
