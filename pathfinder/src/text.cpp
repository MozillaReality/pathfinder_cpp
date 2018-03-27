#include "text.h"

#include <kraken-math.h>

using namespace std;
using namespace kraken;

namespace pathfinder {

PathfinderFont::PathfinderFont()
 : mFace(nullptr)
{

}

bool
PathfinderFont::load(FT_Library aLibrary, const __uint8_t* aData, size_t aDataLength, const std::string& aBuiltinFontName)
  : mBuiltinFontName(aBuiltinFontName)
  , mFace(nullptr)
{
  FT_Error err;
  err = FT_New_Memory_Face(library, aData, aDataLength, 0, &mFace);
  if (err) {
    return false;
  }
}

std::string&
PathfinderFont::getBuiltinFontName()
{
  return mBuiltinFontName;
}

Metrics&
PathfinderFont::metricsForGlyph(int glyphID)
{
  if (mMetricsCache[glyphID] == null)
      mMetricsCache[glyphID] = this.opentypeFont.glyphs.get(glyphID).getMetrics();
  return mMetricsCache[glyphID];
}

UnitMetrics::UnitMetrics(Metrics& metrics, float rotationAngle, kraken::Vector2& emboldenAmount)
{
  float left = metrics.xMin;
  float bottom = metrics.yMin;
  float right = metrics.xMax + emboldenAmount[0] * 2;
  float top = metrics.yMax + emboldenAmount[1] * 2;

  const transform = glmatrix.mat2.create();
  glmatrix.mat2.fromRotation(transform, -rotationAngle);

  Vector2 lowerLeft = Vector2::Max();
  Vector2 upperRight = Vector2::Min();
  const points = [[left, bottom], [left, top], [right, top], [right, bottom]];
  Vector2 transformedPoint = Vector2::Zero();
  for (const point of points) {
    glmatrix.vec2.transformMat2(transformedPoint, point, transform);
    glmatrix.vec2.min(lowerLeft, lowerLeft, transformedPoint);
    glmatrix.vec2.max(upperRight, upperRight, transformedPoint);
  }

  mLeft = lowerLeft[0];
  mRight = upperRight[0];
  mAscent = upperRight[1];
  mDescent = lowerLeft[1];
}

} // namespace pathfinder
