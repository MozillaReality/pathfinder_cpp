#ifndef PATHFINDER_UTILS_H
#define PATHFINDER_UTILS_H

namespace pathfinder {

const int FLOAT32_SIZE = 4;
const int UINT16_SIZE = 2;
const int UINT8_SIZE = 1;

//const int UINT32_MAX = 0xffffffff;
const int UINT32_SIZE = 4;

const float EPSILON = 0.001;

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
