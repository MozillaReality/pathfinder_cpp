// pathfinder/src/pathfinder.cpp
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include "../include/pathfinder.h"

#include "platform.h"

#include "pathfinder-impl.h"

using namespace std;
using namespace kraken;

namespace pathfinder {

TextView::TextView()
{
  mImpl = new TextViewImpl();
}

TextView::~TextView()
{
  delete mImpl;
}

void
TextView::setText(const std::string& aText)
{
  mImpl->setText(aText);
}

std::string
TextView::getText() const
{
  return mImpl->getText();
}

void
TextView::setFont(std::shared_ptr<Font> aFont)
{
  mImpl->setFont(aFont);
}

std::shared_ptr<Font>
TextView::getFont()
{
  return mImpl->getFont();
}

void
TextView::setFontSize(float aFontSize)
{
  mImpl->setFontSize(aFontSize);
}

float
TextView::getFontSize() const
{
  return mImpl->getFontSize();
}

void TextView::setEmboldenAmount(float aEmboldenAmount)
{
  mImpl->setEmboldenAmount(aEmboldenAmount);
}

float TextView::getEmboldenAmount() const
{
  return mImpl->getEmboldenAmount();
}

void
TextView::setRotationAngle(float aRotationAngle)
{
  mImpl->setRotationAngle(aRotationAngle);
}

float
TextView::getRotationAngle() const
{
  return mImpl->getRotationAngle();
}

void
TextView::redraw()
{
  mImpl->redraw();
}

bool
TextView::init()
{
  return mImpl->init();
}

Font::Font()
{
  mImpl = new FontImpl();
}

bool
Font::load(const unsigned char* aData, size_t aDataLength)
{
  return mImpl->load(aData, aDataLength);
}

Font::~Font()
{
  delete mImpl;
}

} // namespace pathfinder
