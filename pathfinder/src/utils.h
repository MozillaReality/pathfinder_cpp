#ifndef PATHFINDER_UTILS_H
#define PATHFINDER_UTILS_H

namespace pathfinder {

class Range {
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
