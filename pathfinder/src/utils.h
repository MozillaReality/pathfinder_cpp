// pathfinder/src/utils.h
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#ifndef PATHFINDER_UTILS_H
#define PATHFINDER_UTILS_H

#include "platform.h"

namespace pathfinder {

const int FLOAT32_SIZE = 4;
const int UINT16_SIZE = 2;
const int UINT8_SIZE = 1;

//const int UINT32_MAX = 0xffffffff;
const int UINT32_SIZE = 4;

const float EPSILON = 0.001f;

constexpr __uint32_t fourcc(char const p[5])
{
  return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

class Range {
public:
  int start;
  int end;

  bool isEmpty()
  {
    return start >= end;
  }

  int length()
  {
    return end - start;
  }

  Range(int aStart, int aEnd)
   : start(aStart)
   , end(aEnd)
  {

  }
};

} // namespace pathfinder

#endif // PATHFINDER_UTILS_H
