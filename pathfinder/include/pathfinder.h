// pathfinder/src/pathfinder.h
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#ifndef PATHFINDER_H
#define PATHFINDER_H

#include <string>
#include <kraken-math.h>

namespace pathfinder {

class FontImpl;
class TextViewImpl;

class Font
{
public:
  Font();
  ~Font();
  bool load(const unsigned char* aData, size_t aDataLength);
private:
  FontImpl* mImpl;

  friend class TextViewImpl;
}; // class Font

class TextView
{
public:
  TextView();
  virtual ~TextView();

  void redraw();
  bool init();

  void setText(const std::string& aText);
  std::string getText() const;
  void setFont(std::shared_ptr<Font> aFont);
  std::shared_ptr<Font> getFont();
  void setFontSize(float aFontSize);
  float getFontSize() const;
  void setEmboldenAmount(float aEmboldenAmount);
  float getEmboldenAmount() const;
  void setRotationAngle(float aRotationAngle);
  float getRotationAngle() const;
private:
  TextViewImpl* mImpl;
}; // class TextView

} // namespace pathfinder

#endif // PATHFINDER_H

