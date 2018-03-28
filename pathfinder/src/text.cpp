#include "text.h"

#include <kraken-math.h>
#include <freetype/ftglyph.h>
#include <locale>
#include <codecvt>

using namespace std;
using namespace kraken;

namespace pathfinder {

PathfinderFont::PathfinderFont()
 : mFace(nullptr)
{

}

bool
PathfinderFont::load(FT_Library aLibrary, const __uint8_t* aData, size_t aDataLength, const std::string& aBuiltinFontName)
{
  mBuiltinFontName = aBuiltinFontName;
  FT_Error err = FT_New_Memory_Face(aLibrary, aData, aDataLength, 0, &mFace);
  if (err) {
    return false;
  }
  return true;
}

std::string&
PathfinderFont::getBuiltinFontName()
{
  return mBuiltinFontName;
}

FT_Face
PathfinderFont::getFreeTypeFont()
{
  return mFace;
}

FT_BBox&
PathfinderFont::metricsForGlyph(int glyphID)
{
  // TODO(kearwood) - Error handling
  std::map<int, FT_BBox>::iterator itr = mMetricsCache.find(glyphID);
  if(itr != mMetricsCache.end()) {
    return itr->second;
  }
  FT_BBox bbox;
  FT_Error err;
  err = FT_Load_Glyph(
    mFace,          /* handle to face object */
    glyphID,   /* glyph index           */
    FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE);  /* load flags, see below */

  FT_Glyph g = nullptr;
  err = FT_Get_Glyph(mFace->glyph, &g);
  FT_Glyph_Get_CBox(g, FT_GLYPH_BBOX_UNSCALED, &bbox);
  FT_Done_Glyph(g);
  mMetricsCache[glyphID] = bbox;
  return mMetricsCache[glyphID];
}

UnitMetrics::UnitMetrics(FT_BBox& metrics, float rotationAngle, kraken::Vector2& emboldenAmount)
{
  float left = metrics.xMin;
  float bottom = metrics.yMin;
  float right = metrics.xMax + emboldenAmount[0] * 2;
  float top = metrics.yMax + emboldenAmount[1] * 2;

  Matrix2 transform = Matrix2::Rotation(-rotationAngle);

  Vector2 lowerLeft = Vector2::Max();
  Vector2 upperRight = Vector2::Min();
  Vector2 points[] = {
    {left, bottom},
    {left, top},
    {right, top},
    {right, bottom}
  };

  for (int i=0; i<4; i++) {
    Vector2 transformedPoint = Matrix2::Dot(transform, transformedPoint);
    lowerLeft = Vector2::Min(lowerLeft, transformedPoint);
    upperRight = Vector2::Max(upperRight, transformedPoint);
  }

  mLeft = lowerLeft[0];
  mRight = upperRight[0];
  mAscent = upperRight[1];
  mDescent = lowerLeft[1];
}

TextRun::TextRun(const string& aText, Vector2 aOrigin, shared_ptr<PathfinderFont> aFont)
  : mOrigin(aOrigin)
  , mFont(aFont)
{
#if _MSC_VER >= 1900
  // Workaround missing symbols in MSVC 2015+ (VSO#143857)
  std::wstring str16 = std::wstring_convert<
    std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.from_bytes(aText);
#else
  std::u16string str16 = std::wstring_convert<
    std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(aText);
#endif

  size_t length = str16.length();
  mGlyphIDs.reserve(length);

  FT_Face face = aFont->getFreeTypeFont();
  for(int i=0; i < length; i++) {
    mGlyphIDs[i] = FT_Get_Char_Index(face, str16[i]);
  }
}

vector<int>&
TextRun::getGlyphIDs()
{
  return mGlyphIDs;
}

std::vector<int>&
TextRun::getAdvances()
{
  return mAdvances;
}

Vector2
TextRun::getOrigin()
{
  return mOrigin;
}

std::shared_ptr<PathfinderFont>
TextRun::getFont()
{
  return mFont;
}

void
TextRun::layout()
{
  mAdvances.clear();
  mAdvances.reserve(mGlyphIDs.size());
  float currentX = 0;
  FT_Face face = mFont->getFreeTypeFont();
  for (int glyphID : mGlyphIDs) {
    mAdvances.push_back(currentX);
    // TODO(kearwood) - Error handling
    FT_Glyph g = nullptr;
    FT_Error err = FT_Get_Glyph(face->glyph, &g);
    currentX += g->advance.x;
    FT_Done_Glyph(g);
  }
}

kraken::Vector2
TextRun::calculatePixelOriginForGlyphAt(int index,
                                        float pixelsPerUnit,
                                        float rotationAngle,
                                        Hint hint,
                                        Vector4 textFrameBounds)
{
  Vector2 textFrameCenter = Vector2::Create(
    0.5f * (textFrameBounds[0] + textFrameBounds[2]),
    0.5f * (textFrameBounds[1] + textFrameBounds[3])
  );

  Matrix2x3 transform = Matrix2x3::Translation(textFrameCenter);
  transform.rotate(-rotationAngle);
  transform.translate(-textFrameCenter);

  Vector2 textGlyphOrigin = Vector2::Create(mAdvances[index], 0.0f) + mOrigin;
  textGlyphOrigin = Matrix2x3::Dot(transform, textGlyphOrigin);
  textGlyphOrigin *= pixelsPerUnit;

  return textGlyphOrigin;
}


kraken::Vector4
TextRun::pixelRectForGlyphAt(int index)
{
  return mPixelRects[index];
}

int
TextRun::subpixelForGlyphAt(int index,
                            float pixelsPerUnit,
                            float rotationAngle,
                            Hint hint,
                            float subpixelGranularity,
                            kraken::Vector4 textFrameBounds)
{
  float textGlyphOrigin = calculatePixelOriginForGlyphAt(index,
                                                         pixelsPerUnit,
                                                         rotationAngle,
                                                         hint,
                                                         textFrameBounds)[0];
  return abs((int)roundf(textGlyphOrigin * subpixelGranularity) % (int)subpixelGranularity);
}

void
TextRun::recalculatePixelRects(float pixelsPerUnit,
                               float rotationAngle,
                               Hint hint,
                               kraken::Vector2 emboldenAmount,
                               float subpixelGranularity,
                               kraken::Vector4 textFrameBounds)
{
  for (int index = 0; index < mGlyphIDs.size(); index++) {
    FT_BBox metrics = mFont->metricsForGlyph(mGlyphIDs[index]);
    UnitMetrics unitMetrics(metrics, rotationAngle, emboldenAmount);
    Vector2 textGlyphOrigin = calculatePixelOriginForGlyphAt(index,
                                                             pixelsPerUnit,
                                                             rotationAngle,
                                                             hint,
                                                             textFrameBounds);
    textGlyphOrigin[0] *= subpixelGranularity;
    textGlyphOrigin = Vector2::Create(round(textGlyphOrigin.x), round(textGlyphOrigin.y));
    textGlyphOrigin[0] /= subpixelGranularity;

    Vector4 pixelRect = calculatePixelRectForGlyph(unitMetrics,
      textGlyphOrigin,
      pixelsPerUnit,
      hint);

    mPixelRects[index] = pixelRect;
  }
}

float
TextRun::measure() {
  if (mGlyphIDs.empty() || mAdvances.empty()) {
    return 0.0f;
  }
  int lastGlyphID = mGlyphIDs.back();
  int advance = mAdvances.back();

  FT_Glyph g = nullptr;
  // TODO(kearwood) - Error handling
  FT_Error err = FT_Get_Glyph(mFont->getFreeTypeFont()->glyph, &g);
  advance += g->advance.x;
  FT_Done_Glyph(g);

  return advance;
}

} // namespace pathfinder
