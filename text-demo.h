// text-demo.h
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#ifndef PATHFINDER_TEXT_DEMO
#define PATHFINDER_TEXT_DEMO

#include <kraken-math.h>
#include <string>

class GLFWwindow;

class TextDemo
{
public:
  TextDemo();
  ~TextDemo();
  void run();
private:
  bool init();
  void renderFrame();
  void shutdown();

  GLFWwindow* mWindow;
  std::string mText;
  float mFontSize;
  float mEmboldenAmount;
  float mRotationAngle;
};

#endif // PATHFINDER_TEXT_DEMO
