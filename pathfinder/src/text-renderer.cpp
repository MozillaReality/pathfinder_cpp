// pathfinder/src/text-renderer.cpp
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include "text-renderer.h"
#include "text.h"
#include "platform.h"
#include "aa-strategy.h"
#include "ssaa-strategy.h"
#include "xcaa-strategy.h"

#include <math.h>

using namespace std;

namespace pathfinder {

const float SQRT_1_2 = 1.0f / sqrtf(2.0f);

TextRenderer::TextRenderer(std::shared_ptr<RenderContext> aRenderContext)
  : Renderer(aRenderContext)
  , mAtlasFramebuffer(0)
  , mAtlasDepthTexture(0)
  , mGlyphPositionsBuffer(0)
  , mGlyphTexCoordsBuffer(0)
  , mGlyphElementsBuffer(0)
  , mFontSize(72.0f)
  , mExtraEmboldenAmount(0.0f)
  , mUseHinting(false)
  , mRotationAngle(0.0f)
  , mDirtyConfig(true)
{
  mAtlas = make_shared<Atlas>();
}


void
TextRenderer::setText(const std::string& aText)
{
  mText = aText;
  mDirtyConfig = true;
}

std::string
TextRenderer::getText() const
{
  return mText;
}

bool
TextRenderer::getIsMulticolor() const
{
  return false;
}

bool
TextRenderer::getNeedsStencil() const
{
  return mFontSize <= MAX_STEM_DARKENING_PIXELS_PER_EM;
}

GLuint
TextRenderer::getDestFramebuffer() const {
  return mAtlasFramebuffer;
}

kraken::Vector2i
TextRenderer::getDestAllocatedSize() const {
  return ATLAS_SIZE;
}

kraken::Vector2i
TextRenderer::getDestUsedSize() const {
  return mAtlas->getUsedSize();
}

kraken::Vector2
TextRenderer::getTotalEmboldenAmount() const {
  return getExtraEmboldenAmount() + getStemDarkeningAmount();
}

kraken::Vector4
TextRenderer::getBGColor() const {
  return Vector4::Create(0.0f, 0.0f, 0.0f, 1.0f);
}

kraken::Vector4
TextRenderer::getFGColor() const {
  return Vector4::One();
}

float
TextRenderer::getRotationAngle() const {
  return mRotationAngle;
}

void
TextRenderer::setRotationAngle(float aRotationAngle)
{
  mRotationAngle = aRotationAngle;
  mDirtyConfig = true;
}

float
TextRenderer::getPixelsPerUnit() const
{
  return mFontSize / mFont->getFreeTypeFont()->units_per_EM;
}

kraken::Matrix4
TextRenderer::getWorldTransform() const
{
  Matrix4 transform = Matrix4::Identity();
  transform.translate(-1.0f, -1.0f, 0.0f);
  transform.scale(2.0f / ATLAS_SIZE[0], 2.0f / ATLAS_SIZE[1], 1.0f);
  return transform;
}

kraken::Vector2
TextRenderer::getStemDarkeningAmount() const
{
  if (mStemDarkening == sdm_dark) {
    return computeStemDarkeningAmount(mFontSize, getPixelsPerUnit());
  }
  return Vector2::Zero();
}

kraken::Vector2
TextRenderer::getUsedSizeFactor() const
{
  Vector2i usedSize = mAtlas->getUsedSize();
  return Vector2::Create(usedSize.x / ATLAS_SIZE.x, usedSize.y / ATLAS_SIZE.y);
}

int
TextRenderer::getPathCount()
{
  return mGlyphStore->getGlyphIDs().size() * SUBPIXEL_GRANULARITY;
}

int
TextRenderer::getObjectCount() const
{
  return getMeshBuffers().size();
}

void
TextRenderer::setHintsUniform(UniformMap& uniforms)
{
  shared_ptr<Hint> hint = createHint();
  glUniform4f(uniforms["uHints"],
              hint->getXHeight(),
              hint->getHintedXHeight(),
              hint->getStemHeight(),
              hint->getHintedStemHeight());
}

float*
TextRenderer::pathBoundingRects(int objectIndex)
{
  float* boundingRects = new float((getPathCount() + 1) * 4);

  for (const AtlasGlyph& glyph: mAtlasGlyphs) {
    const FT_BBox& atlasGlyphMetrics = mFont->metricsForGlyph(glyph.getGlyphKey().getID());
    // TODO(kearwood) error handling needed if FT_Bbox could not be populated?  Origin code "continue"'ed
    UnitMetrics atlasUnitMetrics(atlasGlyphMetrics, 0.0f, getTotalEmboldenAmount());

    int pathID = glyph.getPathID();
    boundingRects[pathID * 4 + 0] = atlasUnitMetrics.mLeft;
    boundingRects[pathID * 4 + 1] = atlasUnitMetrics.mDescent;
    boundingRects[pathID * 4 + 2] = atlasUnitMetrics.mRight;
    boundingRects[pathID * 4 + 3] = atlasUnitMetrics.mAscent;
  }

  return boundingRects;
}

int
TextRenderer::pathBoundingRectsLength(int objectIndex)
{
  return ((getPathCount() + 1) * 4) * sizeof(float);
}

void
TextRenderer::createAtlasFramebuffer()
{
  GLuint atlasColorTexture = mAtlas->ensureTexture(*mRenderContext);
  mAtlasDepthTexture = createFramebufferDepthTexture(ATLAS_SIZE);
  mAtlasFramebuffer = createFramebuffer(atlasColorTexture, mAtlasDepthTexture);

  // Allow the antialiasing strategy to set up framebuffers as necessary.
  if (mAntialiasingStrategy) {
    mAntialiasingStrategy->setFramebufferSize(*this);
  }
}

std::shared_ptr<AntialiasingStrategy>
TextRenderer::createAAStrategy(AntialiasingStrategyName aaType,
                               int aaLevel,
                               SubpixelAAType subpixelAA,
                               StemDarkeningMode stemDarkening)
{
  mSubpixelAA = subpixelAA;
  mStemDarkening = stemDarkening;
  switch (aaType) {
  case asn_none:
    return make_shared<NoAAStrategy>(aaLevel, subpixelAA);
  case asn_ssaa:
    return make_shared<SSAAStrategy>(aaLevel, subpixelAA);
  case asn_xcaa:
    return make_shared<AdaptiveStencilMeshAAAStrategy>(aaLevel, subpixelAA);
    break;
  }
  assert(false);
  return nullptr;
}

bool equalPred(const AtlasGlyph &a, const AtlasGlyph &b)
{
  return a.getGlyphKey().getSortKey() == b.getGlyphKey().getSortKey();
}

void TextRenderer::buildAtlasGlyphs(std::vector<AtlasGlyph> aAtlasGlyphs)
{
  // todo(kearwood) - This may be faster using std::set
  // sort and reduce to unique using predicate
  struct {
      bool operator()(const AtlasGlyph& a, const AtlasGlyph& b) const
      {   
          return a.getGlyphKey().getSortKey() < b.getGlyphKey().getSortKey();
      }
  } comp;
  std::sort(aAtlasGlyphs.begin(), aAtlasGlyphs.end(), comp);
  aAtlasGlyphs.erase(unique(aAtlasGlyphs.begin(), aAtlasGlyphs.end(), equalPred), aAtlasGlyphs.end());
  if (aAtlasGlyphs.empty()) {
    return;
  }

  mAtlasGlyphs = aAtlasGlyphs;
  mAtlas->layoutGlyphs(aAtlasGlyphs,
                           *mFont,
                           getPixelsPerUnit(),
                           mRotationAngle,
                           *createHint(),
                           getTotalEmboldenAmount());

  uploadPathTransforms(1);
  uploadPathColors(1);
}


std::vector<__uint8_t>
TextRenderer::pathColorsForObject(int objectIndex)
{
  int pathCount = getPathCount();

  std::vector<__uint8_t> pathColors;
  pathColors.resize(4 * (pathCount + 1), 0);

  for (int pathIndex = 0; pathIndex < pathCount; pathIndex++) {
    for (int channel = 0; channel < 3; channel++) {
      pathColors[(pathIndex + 1) * 4 + channel] = 0xff; // RGB
    }
    pathColors[(pathIndex + 1) * 4 + 3] = 0xff;         // alpha
  }

  return pathColors;
}

std::shared_ptr<PathTransformBuffers<std::vector<float>>>
TextRenderer::pathTransformsForObject(int objectIndex)
{
  int pathCount = getPathCount();
  int pixelsPerUnit = getPixelsPerUnit();

  // FIXME(pcwalton): This is a hack that tries to preserve the vertical extents of the glyph
  // after stem darkening. It's better than nothing, but we should really do better.
  //
  // This hack seems to produce *better* results than what macOS does on sans-serif fonts;
  // the ascenders and x-heights of the glyphs are pixel snapped, while they aren't on macOS.
  // But we should really figure out what macOS does…
  const FT_Face& fontFace = mFont->getFreeTypeFont();
  float ascender = fontFace->ascender;
  Vector2 emboldenAmount = getTotalEmboldenAmount();
  float stemDarkeningYScale = (ascender + emboldenAmount[1]) / ascender;

  Vector2 stemDarkeningOffset = emboldenAmount;
  stemDarkeningOffset *= pixelsPerUnit;
  stemDarkeningOffset *= SQRT_1_2;
  stemDarkeningOffset.y *= stemDarkeningYScale;

  Matrix2x3 transform = Matrix2x3::Identity();
  std::shared_ptr<PathTransformBuffers<std::vector<float>>> transforms
    = createPathTransformBuffers(pathCount);

  for (const AtlasGlyph& glyph: mAtlasGlyphs) {
    int pathID = glyph.getPathID();
    Vector2 atlasOrigin = glyph.calculateSubpixelOrigin(pixelsPerUnit);

    transform = Matrix2x3::Identity();
    transform.translate(atlasOrigin);
    transform.translate(stemDarkeningOffset);
    transform.rotate(mRotationAngle);
    transform.scale(pixelsPerUnit, pixelsPerUnit * stemDarkeningYScale);

    (*transforms->st)[pathID * 4 + 0] = transform[0];
    (*transforms->st)[pathID * 4 + 1] = transform[3];
    (*transforms->st)[pathID * 4 + 2] = transform[4];
    (*transforms->st)[pathID * 4 + 3] = transform[5];

    (*transforms->ext)[pathID * 2 + 0] = transform[1];
    (*transforms->ext)[pathID * 2 + 1] = transform[2];
  }

  return transforms;
}

std::shared_ptr<Hint>
TextRenderer::createHint()
{
  return make_shared<Hint>(*mFont,
                           getPixelsPerUnit(),
                           mUseHinting);
}

ProgramID
TextRenderer::getDirectCurveProgramName()
{
  return program_directCurve;
}

ProgramID
TextRenderer::getDirectInteriorProgramName(DirectRenderingMode renderingMode)
{
  return program_directInterior;
}

void
TextRenderer::setFont(std::shared_ptr<PathfinderFont> aFont)
{
  mFont = aFont;
  mDirtyConfig = true;
}

std::shared_ptr<PathfinderFont>
TextRenderer::getFont() const
{
  return mFont;
}

float
TextRenderer::getFontSize() const
{
  return mFontSize;
}

void
TextRenderer::setFontSize(float aFontSize)
{
  mFontSize = aFontSize;
  mDirtyConfig = true;
}

bool
TextRenderer::getUseHinting() const
{
  return mUseHinting;
}

void
TextRenderer::setUseHinting(bool aUseHinting)
{
  mUseHinting = aUseHinting;
  mDirtyConfig = true;
}

std::shared_ptr<GlyphStore>
TextRenderer::getGlyphStore()
{
  return mGlyphStore;
}

void
TextRenderer::layout()
{
  if (!mDirtyConfig) {
    return;
  }
  if (!mFont || mText.empty()) {
    return;
  }

  mDirtyConfig = false;

  recreateLayout();
}

void
TextRenderer::recreateLayout()
{
  mLayout = make_unique<SimpleTextLayout>(mFont, mText);

  std::vector<int> uniqueGlyphIDs;
  uniqueGlyphIDs = mLayout->getTextFrame().allGlyphIDs();

  std::sort(uniqueGlyphIDs.begin(), uniqueGlyphIDs.end());
  uniqueGlyphIDs.erase(unique(uniqueGlyphIDs.begin(), uniqueGlyphIDs.end()), uniqueGlyphIDs.end());

  mGlyphStore = make_unique<GlyphStore>(mFont, uniqueGlyphIDs);
  std::shared_ptr<PathfinderMeshPack> meshPack;
  meshPack = mGlyphStore->partition();

  int glyphCount = uniqueGlyphIDs.size();
  std::vector<int> pathIDs;
  for (int glyphIndex = 0; glyphIndex < glyphCount; glyphIndex++) {
    for (int subpixel = 0; subpixel < SUBPIXEL_GRANULARITY; subpixel++) {
      pathIDs.push_back(glyphIndex + 1);
    }
  }
  vector<shared_ptr<PathfinderPackedMeshes>> meshes;
  meshes.push_back(make_shared<PathfinderPackedMeshes>(*meshPack, pathIDs));
  attachMeshes(meshes);
}

void
TextRenderer::layoutText()
{
  mLayout->layoutRuns();

  Vector4 textBounds = mLayout->getTextFrame().bounds();

  int totalGlyphCount = mLayout->getTextFrame().totalGlyphCount();
  vector<float> glyphPositions(totalGlyphCount * 8);
  vector<__uint32_t> glyphIndices(totalGlyphCount * 6);

  std::shared_ptr<Hint> hint = createHint();
  float pixelsPerUnit = getPixelsPerUnit();

  int globalGlyphIndex = 0;
  for (TextRun& run: mLayout->getTextFrame().getRuns()) {
    run.recalculatePixelRects(pixelsPerUnit,
                              mRotationAngle,
                              *hint,
                              getTotalEmboldenAmount(),
                              SUBPIXEL_GRANULARITY,
                              textBounds);

    for (int glyphIndex = 0;
       glyphIndex < run.getGlyphIDs().size();
       glyphIndex++, globalGlyphIndex++) {
      const Vector4 rect = run.pixelRectForGlyphAt(glyphIndex);
      glyphPositions[globalGlyphIndex * 8 + 0] = rect[0];
      glyphPositions[globalGlyphIndex * 8 + 1] = rect[3];
      glyphPositions[globalGlyphIndex * 8 + 2] = rect[2];
      glyphPositions[globalGlyphIndex * 8 + 3] = rect[3];
      glyphPositions[globalGlyphIndex * 8 + 4] = rect[0];
      glyphPositions[globalGlyphIndex * 8 + 5] = rect[1];
      glyphPositions[globalGlyphIndex * 8 + 6] = rect[2];
      glyphPositions[globalGlyphIndex * 8 + 7] = rect[1];

      for (int glyphIndexIndex = 0;
        glyphIndexIndex < QUAD_ELEMENTS_LENGTH;
        glyphIndexIndex++) {
        glyphIndices[glyphIndexIndex + globalGlyphIndex * 6] =
            QUAD_ELEMENTS[glyphIndexIndex] + 4 * globalGlyphIndex;
      }
    }
  }

  glCreateBuffers(1, &mGlyphPositionsBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, mGlyphPositionsBuffer);
  glBufferData(GL_ARRAY_BUFFER, glyphPositions.size() * sizeof(glyphPositions[0]), &glyphPositions[0], GL_STATIC_DRAW);

  glCreateBuffers(1, &mGlyphElementsBuffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mGlyphElementsBuffer);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, glyphIndices.size() * sizeof(glyphIndices[0]), &glyphIndices[0], GL_STATIC_DRAW);
}

/// The separating axis theorem.
bool rectsIntersect(Vector4 a, Vector4 b)
{
    return a[2] > b[0] && a[3] > b[1] && a[0] < b[2] && a[1] < b[3];
}

void
TextRenderer::buildGlyphs(Vector2 aViewTranslation, Vector2 aViewSize)
{
  float pixelsPerUnit = getPixelsPerUnit();
  Vector4 textBounds = mLayout->getTextFrame().bounds();
  std::shared_ptr<Hint> hint = createHint();

  // Only build glyphs in view.
  Vector2 translation = aViewTranslation;
  Vector4 canvasRect = Vector4::Create(
    -translation[0],
    -translation[1],
    -translation[0] + aViewSize.x,
    -translation[1] + aViewSize.y
  );

  std::vector<AtlasGlyph> atlasGlyphs;
  for (TextRun& run: mLayout->getTextFrame().getRuns()) {
      for (int glyphIndex = 0; glyphIndex < run.getGlyphIDs().size(); glyphIndex++) {
          // TODO(kearwood) - HACK - re-enable culling code and fix crash
          // when all glyphs have been culled.
          const Vector4 pixelRect = run.pixelRectForGlyphAt(glyphIndex);
          if (!rectsIntersect(pixelRect, canvasRect)) {
            continue;
          }

          int glyphID = run.getGlyphIDs()[glyphIndex];
          int glyphStoreIndex = mGlyphStore->indexOfGlyphWithID(glyphID);
          if (glyphStoreIndex == -1) {
            continue;
          }

          int subpixel = run.subpixelForGlyphAt(glyphIndex,
                                                pixelsPerUnit,
                                                mRotationAngle,
                                                *hint,
                                                SUBPIXEL_GRANULARITY,
                                                textBounds);
          GlyphKey glyphKey(glyphID, true, subpixel);
          atlasGlyphs.push_back(AtlasGlyph(glyphStoreIndex, glyphKey));
      }
  }

  buildAtlasGlyphs(atlasGlyphs);

  // TODO(pcwalton): Regenerate the IBOs to include only the glyphs we care about.
  setGlyphTexCoords();
}

void
TextRenderer::setGlyphTexCoords()
{
  float pixelsPerUnit = getPixelsPerUnit();

  Vector4 textBounds = mLayout->getTextFrame().bounds();

  shared_ptr<Hint> hint = createHint();

  mGlyphBounds.resize(mLayout->getTextFrame().totalGlyphCount() * 8);

  int globalGlyphIndex = 0;
  for (TextRun& run: mLayout->getTextFrame().getRuns()) {
    for (int glyphIndex = 0;
       glyphIndex < run.getGlyphIDs().size();
       glyphIndex++, globalGlyphIndex++) {
      int textGlyphID = run.getGlyphIDs()[glyphIndex];

      int subpixel = run.subpixelForGlyphAt(glyphIndex,
                                            pixelsPerUnit,
                                            mRotationAngle,
                                            *hint,
                                            SUBPIXEL_GRANULARITY,
                                            textBounds);
      // TODO(kearwood) - This is a hack!  Sometimes hasSubpixel should be false...
      bool hasSubpixel = true;
      GlyphKey glyphKey(textGlyphID, hasSubpixel, subpixel);
      int sortKey = glyphKey.getSortKey();

      // Find index of glyphKey in mAtlasGlyphs, assuming mAtlasGlyphs is sorted by sortkey
      // TODO(kearwood) - This is slow...
      AtlasGlyph* atlasGlyph = nullptr;
      for(AtlasGlyph& g: mAtlasGlyphs) {
        if (g.getGlyphKey().getSortKey() == sortKey) {
          atlasGlyph = &g;
          break;
        }
      }
      if (atlasGlyph == nullptr) {
        break;
      }
      // Set texture coordinates.
      const FT_BBox& atlasGlyphMetrics = mFont->metricsForGlyph(atlasGlyph->getGlyphKey().getID());

      UnitMetrics atlasGlyphUnitMetrics(atlasGlyphMetrics,
                                        mRotationAngle,
                                        getTotalEmboldenAmount());

      Vector2 atlasGlyphPixelOrigin =
          atlasGlyph->calculateSubpixelOrigin(pixelsPerUnit);
      Vector4 atlasGlyphRect =
          calculatePixelRectForGlyph(atlasGlyphUnitMetrics,
                                     atlasGlyphPixelOrigin,
                                     pixelsPerUnit,
                                     *hint);
      Vector2 atlasGlyphBL = Vector2::Create(atlasGlyphRect[0], atlasGlyphRect[1]);
      Vector2 atlasGlyphTR = Vector2::Create(atlasGlyphRect[2], atlasGlyphRect[3]);
      atlasGlyphBL.x /= (float)ATLAS_SIZE.x;
      atlasGlyphBL.y /= (float)ATLAS_SIZE.y;
      atlasGlyphTR.x /= (float)ATLAS_SIZE.x;
      atlasGlyphTR.y /= (float)ATLAS_SIZE.y;

      mGlyphBounds[globalGlyphIndex * 8 + 0] = atlasGlyphBL[0];
      mGlyphBounds[globalGlyphIndex * 8 + 1] = atlasGlyphTR[1];
      mGlyphBounds[globalGlyphIndex * 8 + 2] = atlasGlyphTR[0];
      mGlyphBounds[globalGlyphIndex * 8 + 3] = atlasGlyphTR[1];
      mGlyphBounds[globalGlyphIndex * 8 + 4] = atlasGlyphBL[0];
      mGlyphBounds[globalGlyphIndex * 8 + 5] = atlasGlyphBL[1];
      mGlyphBounds[globalGlyphIndex * 8 + 6] = atlasGlyphTR[0];
      mGlyphBounds[globalGlyphIndex * 8 + 7] = atlasGlyphBL[1];
    }
  }

  glCreateBuffers(1, &mGlyphTexCoordsBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, mGlyphTexCoordsBuffer);
  glBufferData(GL_ARRAY_BUFFER, mGlyphBounds.size() * sizeof(mGlyphBounds[0]), &mGlyphBounds[0], GL_STATIC_DRAW);
}


void
TextRenderer::compositeIfNecessary(Vector2 aViewTranslation, Vector2 aViewSize)
{
  // Set up composite state.
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, aViewSize.x, aViewSize.y);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_SCISSOR_TEST);
  glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
  glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE);
  glEnable(GL_BLEND);

  // Clear.
  glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // Set the appropriate program.
  shared_ptr<PathfinderShaderProgram> blitProgram;
  switch (mGammaCorrectionMode) {
  case gcm_off:
    blitProgram = mRenderContext->getShaderManager().getProgram(program_blitLinear);
    break;
  case gcm_on:
    blitProgram = mRenderContext->getShaderManager().getProgram(program_blitGamma);
    break;
  }

  // Set up the composite VAO.
  glUseProgram(blitProgram->getProgram());
  glBindBuffer(GL_ARRAY_BUFFER, mGlyphPositionsBuffer);
  glVertexAttribPointer(blitProgram->getAttributes()["aPosition"], 2, GL_FLOAT, GL_FALSE, 0, 0);
  glBindBuffer(GL_ARRAY_BUFFER, mGlyphTexCoordsBuffer);
  glVertexAttribPointer(blitProgram->getAttributes()["aTexCoord"], 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(blitProgram->getAttributes()["aPosition"]);
  glEnableVertexAttribArray(blitProgram->getAttributes()["aTexCoord"]);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mGlyphElementsBuffer);

  // Create the transform.
  Matrix4 transform = Matrix4::Identity();
  transform.translate(-1.0f, -1.0f, 0.0f);
  transform.scale(
    2.0 / aViewSize.x,
    2.0 / aViewSize.y,
    1.0
  );
  transform.translate(
    aViewTranslation[0],
    aViewTranslation[1],
    0.0f
  );

  // Blit.
  glUniformMatrix4fv(blitProgram->getUniforms()["uTransform"], 1, GL_FALSE, transform.c);
  glActiveTexture(GL_TEXTURE0);
  GLuint destTexture = mAtlas->ensureTexture(*mRenderContext);
  glBindTexture(GL_TEXTURE_2D, destTexture);
  glUniform1i(blitProgram->getUniforms()["uSource"], 0);
  glUniform2f(blitProgram->getUniforms()["uTexScale"], 1.0, 1.0);
  bindGammaLUT(Vector3::Create(1.0f, 1.0f, 1.0f), 1, blitProgram->getUniforms());
  int totalGlyphCount = mLayout->getTextFrame().totalGlyphCount();
  glDrawElements(GL_TRIANGLES, totalGlyphCount * 6, GL_UNSIGNED_INT, 0);
}

kraken::Vector2
TextRenderer::getExtraEmboldenAmount() const
{
  const FT_Face& face = mFont->getFreeTypeFont();
  float emboldenLength = mExtraEmboldenAmount * face->units_per_EM;
  return Vector2::Create(emboldenLength, emboldenLength);
}

void
TextRenderer::setEmboldenAmount(float aEmboldenAmount)
{
  mExtraEmboldenAmount = aEmboldenAmount;
  mDirtyConfig = true;
}
float
TextRenderer::getEmboldenAmount() const
{
  return mExtraEmboldenAmount;
}

void
TextRenderer::prepareToAttachText()
{
  if (mAtlasFramebuffer == 0) {
    createAtlasFramebuffer();
  }
  layoutText();
}

} // namespace pathfinder
