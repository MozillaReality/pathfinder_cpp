// pathfinder/src/pathfinder.cpp
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.


#include "pathfinder-impl.h"

#include "platform.h"
#include "gl-utils.h"
#include "atlas.h"
#include "shader-loader.h"

using namespace std;
using namespace kraken;

namespace pathfinder {

TextViewImpl::TextViewImpl()
  : mCameraTranslation(Vector2::Create(0.0f, 768.0f))
  , mCameraViewSize(Vector2::Create(1024.0f, 768.0f))
{
}

TextViewImpl::~TextViewImpl()
{
}

void
TextViewImpl::setText(const std::string& aText)
{
  mRenderer->setText(aText);
}

std::string
TextViewImpl::getText() const
{
  return mRenderer->getText();
}

void
TextViewImpl::setFontSize(float aFontSize)
{
  mRenderer->setFontSize(aFontSize);
}

float
TextViewImpl::getFontSize() const
{
  return mRenderer->getFontSize();
}

void TextViewImpl::setEmboldenAmount(float aEmboldenAmount)
{
  mRenderer->setEmboldenAmount(aEmboldenAmount);
}

float TextViewImpl::getEmboldenAmount() const
{
  return mRenderer->getEmboldenAmount();
}

void
TextViewImpl::setRotationAngle(float aRotationAngle)
{
  mRenderer->setRotationAngle(aRotationAngle);
}

float TextViewImpl::getRotationAngle() const
{
  return mRenderer->getRotationAngle();
}

bool
TextViewImpl::getUseHinting() const
{
  return mRenderer->getUseHinting();
}

void
TextViewImpl::setUseHinting(bool aUseHinting)
{
  mRenderer->setUseHinting(aUseHinting);
}

void
TextViewImpl::setFont(std::shared_ptr<Font> aFont)
{
  mFont = aFont;
  mRenderer->setFont(aFont->mImpl->getFont());
}

std::shared_ptr<Font>
TextViewImpl::getFont() const
{
  return mFont;
}

void
TextViewImpl::redraw()
{
  mRenderer->layout();

  if (!mRenderer->getMeshesAttached()) {
    return;
  }
  mRenderer->buildGlyphs(mCameraTranslation, mCameraViewSize);
  mRenderer->redraw(mCameraTranslation, mCameraViewSize);
}

bool
TextViewImpl::init()
{
  AAOptions options;
  options.gammaCorrection = gcm_on;
  options.stemDarkening = sdm_dark;
  options.subpixelAA = saat_none;

  mRenderContext = make_shared<RenderContext>();
  if (!mRenderContext->init()) {
    return false;
  }
  mRenderer = make_shared<TextRenderer>(mRenderContext);
  if (!mRenderer->init(asn_xcaa, 1, options)) {
    return false;
  }
  return true;
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
