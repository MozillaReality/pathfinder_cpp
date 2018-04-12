#include "text.h"
#include "resources.h"

#include <kraken-math.h>
#include <freetype/ftglyph.h>
#include <freetype/tttables.h>
#include <locale>
#include <algorithm>
#include <codecvt>
#include <assert.h>
#include <sstream>
#include <iostream>

using namespace std;
using namespace kraken;

namespace pathfinder {

PathfinderFont::PathfinderFont()
 : mFace(nullptr)
{

}

bool
PathfinderFont::load(FT_Library aLibrary, const __uint8_t* aData, size_t aDataLength)
{
  FT_Error err = FT_New_Memory_Face(aLibrary, aData, aDataLength, 0, &mFace);
  if (err) {
    return false;
  }
  return true;
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

UnitMetrics::UnitMetrics(const FT_BBox& metrics, float rotationAngle, const kraken::Vector2& emboldenAmount)
{
  // Converting from 24.6 fixed to float
  float left = (float)metrics.xMin / 64.0f;
  float bottom = (float)metrics.yMin / 64.0f;
  float right = (float)metrics.xMax / 64.0f + emboldenAmount[0] * 2;
  float top = (float)metrics.yMax / 64.0f + emboldenAmount[1] * 2;

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
    Vector2 transformedPoint = Matrix2::Dot(transform, points[i]);
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
  mGlyphIDs.resize(length, 0);

  FT_Face face = aFont->getFreeTypeFont();
  for(int i=0; i < length; i++) {
    mGlyphIDs[i] = FT_Get_Char_Index(face, str16[i]);
  }
}

const vector<int>&
TextRun::getGlyphIDs() const
{
  return mGlyphIDs;
}

const std::vector<float>&
TextRun::getAdvances() const
{
  return mAdvances;
}

const Vector2
TextRun::getOrigin() const
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

    FT_Error err = FT_Load_Glyph(face, glyphID, FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE);
    assert(err == 0);
    FT_Glyph g = nullptr;
    err = FT_Get_Glyph(face->glyph, &g);
    assert(err == 0);
    currentX += (float)g->advance.x / 64.0f; // converting from 24.6 fixed point to float
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
  mPixelRects.resize(mGlyphIDs.size());
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
TextRun::measure() const
{
  if (mGlyphIDs.empty() || mAdvances.empty()) {
    return 0.0f;
  }
  int lastGlyphID = mGlyphIDs.back();
  float advance = mAdvances.back();

  FT_Glyph g = nullptr;
  // TODO(kearwood) - Error handling
  FT_Error err = FT_Get_Glyph(mFont->getFreeTypeFont()->glyph, &g);
  advance += g->advance.x / 64.0f; // converting from 24.6 fixed point to float
  FT_Done_Glyph(g);

  return advance;
}


TextFrame::TextFrame(const vector<TextRun>& aRuns, std::shared_ptr<PathfinderFont> aFont)
  : mRuns(aRuns)
  , mOrigin(Vector3::Zero())
  , mFont(aFont)
{

}

ExpandedMeshData
TextFrame::expandMeshes(const PathfinderMeshPack& meshes, std::vector<int>& glyphIDs)
{
  vector<int> pathIDs;
  for (const TextRun& textRun: mRuns) {
    for (int glyphID: textRun.getGlyphIDs()) {
      if (glyphID == 0) {
        continue;
      }
      // Find index of glyphID in glyphIDs, assuming glyphIDs is sorted
      vector<int>::iterator first = glyphIDs.begin();
      vector<int>::iterator last = glyphIDs.end();
      std::lower_bound(first, last, glyphID);
      // Assert that it was found
      assert(first != last);
      assert(glyphID == *first);

      int pathID = first - glyphIDs.begin();
      pathIDs.push_back(pathID + 1);
    }
  }

  ExpandedMeshData r;
  r.meshes = make_shared<PathfinderPackedMeshes>(meshes, pathIDs);
  return r;
}

vector<TextRun>&
TextFrame::getRuns()
{
  return mRuns;
}

kraken::Vector3
TextFrame::getOrigin() const
{
  return mOrigin;
}

kraken::Vector4
TextFrame::bounds() const
{
  if (mRuns.empty()) {
    return Vector4::Create();
  }
  Vector2 upperLeft = mRuns.front().getOrigin();
  Vector2 lowerRight = mRuns.back().getOrigin();

  Vector2 lowerLeft = Vector2::Create(upperLeft[0], lowerRight[1]);
  Vector2 upperRight = Vector2::Create(lowerRight[0], upperLeft[1]);

  float lineHeight = getFontLineHeight(*mFont);
  lowerLeft[1] -= lineHeight;
  upperRight[1] += lineHeight * 2.0f;

  upperRight[0] = 0.0f;

  for (const TextRun& run : mRuns) {
    float runWidth = run.measure();
    if (runWidth > upperRight[0]) {
      upperRight[0] = runWidth;
    }
  }

  return Vector4::Create(lowerLeft[0], lowerLeft[1], upperRight[0], upperRight[1]);
}

size_t
TextFrame::totalGlyphCount() const
{
  size_t count = 0;
  for (const TextRun& run : mRuns) {
    count += run.getGlyphIDs().size();
  }
  return count;
}

vector<int>
TextFrame::allGlyphIDs() const
{
  vector<int> glyphIds;
  glyphIds.reserve(totalGlyphCount());
  for (const TextRun& run : mRuns) {
    const std::vector<int>& runGlyphIds = run.getGlyphIDs();
    glyphIds.insert(end(glyphIds), begin(runGlyphIds), std::end(runGlyphIds));
  }
  return glyphIds;
}

Hint::Hint(PathfinderFont& aFont, float aPixelsPerUnit, bool aUseHinting)
  : mUseHinting(aUseHinting)
{
  const FT_Face& face = aFont.getFreeTypeFont();
  const TT_OS2* os2Table = (TT_OS2*)FT_Get_Sfnt_Table(face, FT_SFNT_OS2);
  assert(os2Table);
  mXHeight = os2Table->sxHeight;
  mStemHeight = os2Table->sCapHeight;

  if (!mUseHinting) {
    mHintedXHeight = mXHeight;
    mHintedStemHeight = mStemHeight;
  }
  else {
    mHintedXHeight = roundf(roundf(mXHeight * aPixelsPerUnit) / aPixelsPerUnit);
    mHintedStemHeight = roundf(roundf(mStemHeight * aPixelsPerUnit) / aPixelsPerUnit);
  }
}
/// NB: This must match `hintPosition()` in `common.inc.glsl`.
Vector2
Hint::hintPosition(Vector2 position)
{
  if (!mUseHinting) {
    return position;
  }

  if (position[1] >= mStemHeight) {
    float y = position[1] - mStemHeight + mHintedStemHeight;
    return Vector2::Create(position[0], y);
  }

  if (position[1] >= mXHeight) {
    float y = kraken::Lerp(mHintedXHeight, mHintedStemHeight,
      (position[1] - mXHeight) / (mStemHeight - mXHeight));
    return Vector2::Create(position[0], y);
  }

  if (position[1] >= 0.0f) {
    float y = kraken::Lerp(0.0f, mHintedXHeight, position[1] / mXHeight);
    return Vector2::Create(position[0], y);
  }

  return position;
}

float
Hint::getXHeight() const
{
  return mXHeight;
}

float
Hint::getHintedXHeight() const
{
  return mHintedXHeight;
}

float
Hint::getStemHeight() const
{
  return mStemHeight;
}

float
Hint::getHintedStemHeight() const
{
  return mHintedStemHeight;
}

std::shared_ptr<PathfinderFont>
GlyphStore::getFont()
{
  return mFont;
}

const std::vector<int>&
GlyphStore::getGlyphIDs()
{
  return mGlyphIDs;
}

std::shared_ptr<PathfinderMeshPack>
GlyphStore::partition()
{
  // TODO(kearwood) - The partition result is hard-coded for demo purposes.
  //                  the partitioning was implemented server-side in the original
  //                  WebGL demo and should be embedded into the library to replace this
  //                  function.
  shared_ptr<PathfinderMeshPack> meshPack = make_shared<PathfinderMeshPack>();
  meshPack->load((__uint8_t*)partition_font_bin, partition_font_bin_len);
  return meshPack;
}

int
GlyphStore::indexOfGlyphWithID(int glyphID)
{
  // Find index of glyphID in glyphIDs, assuming mGlyphIDs is sorted
  vector<int>::iterator first = mGlyphIDs.begin();
  vector<int>::iterator last = mGlyphIDs.end();
  vector<int>::iterator found = std::lower_bound(first, last, glyphID);
  if (found == last || glyphID != *found) {
    return -1; // not found
  }

  return (int)(found - mGlyphIDs.begin());
}

SimpleTextLayout::SimpleTextLayout(std::shared_ptr<PathfinderFont> aFont, std::string aText)
{
  float lineHeight = getFontLineHeight(*aFont);
  vector<TextRun> textRuns;
  string line;
  istringstream s(aText);
  int lineNumber = 0;
  while (getline(s, line, '\n')) {
    textRuns.push_back(TextRun(line, Vector2::Create(0.0f, -lineHeight * lineNumber), aFont));
    ++lineNumber;
  }
  mTextFrame = make_unique<TextFrame>(textRuns, aFont);
}

TextFrame&
SimpleTextLayout::getTextFrame()
{
  return *mTextFrame;
}

void
SimpleTextLayout::layoutRuns() {
  for (TextRun& run : mTextFrame->getRuns()) {
    run.layout();
  }
}

float
getFontLineHeight(PathfinderFont& aFont)
{
  FT_Face face = aFont.getFreeTypeFont();
  const TT_OS2* os2Table = (TT_OS2*)FT_Get_Sfnt_Table(face, FT_SFNT_OS2);
  assert(os2Table);
  return (float)(os2Table->sTypoAscender - os2Table->sTypoDescender + os2Table->sTypoLineGap);
}

kraken::Vector4
calculatePixelRectForGlyph(const UnitMetrics& metrics,
  kraken::Vector2 subpixelOrigin,
  float pixelsPerUnit,
  const Hint& hint)
{
  PixelMetrics pixelMetrics = calculateSubpixelMetricsForGlyph(metrics, pixelsPerUnit, hint);
  return Vector4::Create(floor(subpixelOrigin[0] + pixelMetrics.left),
    floorf(subpixelOrigin[1] + pixelMetrics.descent),
    ceilf(subpixelOrigin[0] + pixelMetrics.right),
    ceilf(subpixelOrigin[1] + pixelMetrics.ascent));
}

PixelMetrics
calculateSubpixelMetricsForGlyph(const UnitMetrics& metrics, float pixelsPerUnit, Hint hint)
{
  float ascent = hint.hintPosition(Vector2::Create(0.0f, metrics.mAscent))[1];
  PixelMetrics pm;
  pm.left = metrics.mLeft * pixelsPerUnit;
  pm.right = metrics.mRight * pixelsPerUnit;
  pm.ascent = ascent * pixelsPerUnit;
  pm.descent = metrics.mDescent * pixelsPerUnit;
  return pm;
}

kraken::Vector2
computeStemDarkeningAmount(float pixelsPerEm, float pixelsPerUnit)
{
  if (pixelsPerEm > MAX_STEM_DARKENING_PIXELS_PER_EM) {
    return Vector2::Zero();
  }

  return Vector2::Min(STEM_DARKENING_FACTORS * pixelsPerEm, MAX_STEM_DARKENING_AMOUNT) / pixelsPerUnit;
}

float
calculatePixelXMin(const UnitMetrics& metrics, float pixelsPerUnit)
{
  return floorf(metrics.mLeft * pixelsPerUnit);
}

float calculatePixelDescent(const UnitMetrics& metrics, float pixelsPerUnit)
{
  return floorf(metrics.mDescent * pixelsPerUnit);
}

} // namespace pathfinder
