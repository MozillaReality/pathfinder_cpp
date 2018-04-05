// pathfinder/src/pathfinder.cpp
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include "platform.h"

#include "pathfinder-impl.h"
#include "atlas.h"

using namespace std;
using namespace kraken;

namespace pathfinder {

TextViewImpl::TextViewImpl()
  : mFontSize(72.0f)
  , mEmboldenAmount(0.0f)
  , mRotationAngle(0.0f)
  , mDirtyConfig(true)
{
}

TextViewImpl::~TextViewImpl()
{
}

void
TextViewImpl::setText(const std::string& aText)
{
  mText = aText;
  mDirtyConfig = true;
}

std::string
TextViewImpl::getText() const
{
  return mText;
}

void
TextViewImpl::setFontSize(float aFontSize)
{
  mFontSize = aFontSize;
  mDirtyConfig = true;
}

float
TextViewImpl::getFontSize() const
{
  return mFontSize;
}

void TextViewImpl::setEmboldenAmount(float aEmboldenAmount)
{
  mEmboldenAmount = aEmboldenAmount;
  mDirtyConfig = true;
}

float TextViewImpl::getEmboldenAmount() const
{
  return mEmboldenAmount;
}

void
TextViewImpl::setRotationAngle(float aRotationAngle)
{
  mRotationAngle = aRotationAngle;
  mDirtyConfig = true;
}

float TextViewImpl::getRotationAngle() const
{
  return mRotationAngle;
}


void
TextViewImpl::setFont(std::shared_ptr<Font> aFont)
{
  mFont = aFont;
  mDirtyConfig = true;
}


std::shared_ptr<Font>
TextViewImpl::getFont()
{
  return mFont;
}

bool equalPred(const AtlasGlyph &a, const AtlasGlyph &b)
{
  return a.getGlyphKey().getSortKey() == b.getGlyphKey().getSortKey();
}

void
TextViewImpl::recreateLayout()
{
  if (!mDirtyConfig) {
    return;
  }

  mDirtyConfig = false;

  if (!mFont || mText.empty()) {
    return;
  }

  mLayout = make_unique<SimpleTextLayout>(mFont->mImpl->getFont(), mText);

  std::vector<int> uniqueGlyphIDs;
  uniqueGlyphIDs = mLayout->getTextFrame().allGlyphIDs();

  // todo(kearwood) - This may be faster using std::set
  // sort and reduce to unique using predicate
  struct {
      bool operator()(const AtlasGlyph& a, const AtlasGlyph& b) const
      {   
          return a.getGlyphKey().getSortKey() < b.getGlyphKey().getSortKey();
      }
  } comp;
  std::sort(uniqueGlyphIDs.begin(), uniqueGlyphIDs.end(), comp);
  uniqueGlyphIDs.erase(unique(uniqueGlyphIDs.begin(), uniqueGlyphIDs.end(), equalPred), uniqueGlyphIDs.end());

  std::shared_ptr<GlyphStore> glyphStore;
  glyphStore = make_unique<GlyphStore>(mFont->mImpl->getFont(), uniqueGlyphIDs);
  std::shared_ptr<PathfinderMeshPack> meshPack;
  meshPack = glyphStore->partition();
}

FontImpl::FontImpl()
  : mFTLibrary(nullptr)
{
  mFont = make_unique<PathfinderFont>();
}

bool
FontImpl::load(const unsigned char* aData, size_t aDataLength)
{
  if (!mFTLibrary) {
    FT_Error error = FT_Init_FreeType(&mFTLibrary);
    if (error) {
      return false;
    }
  }
  return mFont->load(mFTLibrary, aData, aDataLength);
}

std::shared_ptr<PathfinderFont> 
FontImpl::getFont()
{
  return mFont;
}

FontImpl::~FontImpl()
{
  if (mFTLibrary) {
    FT_Done_FreeType(mFTLibrary);
    mFTLibrary = nullptr;
  }
}

} // namespace pathfinder
