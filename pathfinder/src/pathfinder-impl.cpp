// pathfinder/src/pathfinder.cpp
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include "platform.h"

#include "pathfinder-impl.h"
#include "gl-utils.h"
#include "atlas.h"

using namespace std;
using namespace kraken;

namespace pathfinder {

/// The separating axis theorem.
bool rectsIntersect(Vector4 a, Vector4 b)
{
    return a[2] > b[0] && a[3] > b[1] && a[0] < b[2] && a[1] < b[3];
}

TextViewImpl::TextViewImpl()
  : mFontSize(72.0f)
  , mEmboldenAmount(0.0f)
  , mRotationAngle(0.0f)
  , mDirtyConfig(true)
{
  mAtlas = make_shared<Atlas>();
}

TextViewImpl::~TextViewImpl()
{
}

void
TextViewImpl::setText(const std::string& aText)
{
  mText = aText;
  mDirtyConfig = true;
}

std::string
TextViewImpl::getText() const
{
  return mText;
}

void
TextViewImpl::setFontSize(float aFontSize)
{
  mFontSize = aFontSize;
  mDirtyConfig = true;
}

float
TextViewImpl::getFontSize() const
{
  return mFontSize;
}

void TextViewImpl::setEmboldenAmount(float aEmboldenAmount)
{
  mEmboldenAmount = aEmboldenAmount;
  mDirtyConfig = true;
}

float TextViewImpl::getEmboldenAmount() const
{
  return mEmboldenAmount;
}

void
TextViewImpl::setRotationAngle(float aRotationAngle)
{
  mRotationAngle = aRotationAngle;
  mDirtyConfig = true;
}

float TextViewImpl::getRotationAngle() const
{
  return mRotationAngle;
}

std::shared_ptr<GlyphStore>
TextViewImpl::getGlyphStore()
{
  return mGlyphStore;
}

bool
TextViewImpl::getUseHinting() const
{
  return false;
}

std::vector<AtlasGlyph>
TextViewImpl::getAtlasGlyphs()
{
  return mAtlasGlyphs;
}

void
TextViewImpl::setAtlasGlyphs(const std::vector<AtlasGlyph>& aAtlasGlyphs)
{
  mAtlasGlyphs = aAtlasGlyphs;
}

std::shared_ptr<Atlas>
TextViewImpl::getAtlas()
{
  return mAtlas;
}

SimpleTextLayout&
TextViewImpl::getLayout()
{
  return *mLayout;
}

void
TextViewImpl::setFont(std::shared_ptr<Font> aFont)
{
  mFont = aFont;
  mDirtyConfig = true;
}


std::shared_ptr<PathfinderFont>
TextViewImpl::getFont() const
{
  return mFont->mImpl->getFont();
}

std::shared_ptr<Font>
TextViewImpl::getPublicFont() const
{
  return mFont;
}

void
TextViewImpl::recreateLayout()
{
  if (!mDirtyConfig) {
    return;
  }

  mDirtyConfig = false;

  if (!mFont || mText.empty()) {
    return;
  }

  mLayout = make_unique<SimpleTextLayout>(mFont->mImpl->getFont(), mText);

  std::vector<int> uniqueGlyphIDs;
  uniqueGlyphIDs = mLayout->getTextFrame().allGlyphIDs();

  std::sort(uniqueGlyphIDs.begin(), uniqueGlyphIDs.end());
  uniqueGlyphIDs.erase(unique(uniqueGlyphIDs.begin(), uniqueGlyphIDs.end()), uniqueGlyphIDs.end());

  mGlyphStore = make_unique<GlyphStore>(mFont->mImpl->getFont(), uniqueGlyphIDs);
  std::shared_ptr<PathfinderMeshPack> meshPack;
  meshPack = mGlyphStore->partition();

  int glyphCount = uniqueGlyphIDs.size();
  std::vector<int> pathIDs;
  for (int glyphIndex = 0; glyphIndex < glyphCount; glyphIndex++) {
    for (int subpixel = 0; subpixel < SUBPIXEL_GRANULARITY; subpixel++) {
      pathIDs.push_back(glyphIndex + 1);
    }
  }
  mMeshes = make_shared<PathfinderPackedMeshes>(*meshPack, pathIDs);
}

FontImpl::FontImpl()
  : mFTLibrary(nullptr)
{
  mFont = make_unique<PathfinderFont>();
}

bool
FontImpl::load(const unsigned char* aData, size_t aDataLength)
{
  if (!mFTLibrary) {
    FT_Error error = FT_Init_FreeType(&mFTLibrary);
    if (error) {
      return false;
    }
  }
  return mFont->load(mFTLibrary, aData, aDataLength);
}

std::shared_ptr<PathfinderFont> 
FontImpl::getFont()
{
  return mFont;
}

FontImpl::~FontImpl()
{
  if (mFTLibrary) {
    FT_Done_FreeType(mFTLibrary);
    mFTLibrary = nullptr;
  }
}

TextViewRenderer::TextViewRenderer(std::shared_ptr<TextViewImpl> aTextView)
  : TextRenderer(aTextView)
  , mGlyphPositionsBuffer(0)
  , mGlyphTexCoordsBuffer(0)
  , mGlyphElementsBuffer(0)
  , mCameraTranslation(Vector2::Create(0.0f, 0.0f))
  , mCameraViewSize(Vector2::Create(512.0f, 512.0f))
  , mTextView(aTextView)
{

}

TextViewRenderer::~TextViewRenderer()
{

}

kraken::Vector4
TextViewRenderer::getBackgroundColor() const
{
  return Vector4::Zero();
}


kraken::Vector2
TextViewRenderer::getExtraEmboldenAmount() const
{
  float emboldenLength = 0.0f;
  // TODO(kearwood) - Implement and uncomment:
  // float emboldenLength = appController.emboldenAmount * appController.unitsPerEm;
  return Vector2::Create(emboldenLength, emboldenLength);
}


void
TextViewRenderer::prepareToAttachText()
{
  if (mAtlasFramebuffer == 0) {
    createAtlasFramebuffer();
  }
  layoutText();
}

void
TextViewRenderer::layoutText()
{
  mTextView->getLayout().layoutRuns();

  Vector4 textBounds = mTextView->getLayout().getTextFrame().bounds();

  int totalGlyphCount = mTextView->getLayout().getTextFrame().totalGlyphCount();
  vector<float> glyphPositions(totalGlyphCount * 8);
  vector<__uint32_t> glyphIndices(totalGlyphCount * 6);

  std::shared_ptr<Hint> hint = createHint();
  float pixelsPerUnit = getPixelsPerUnit();
  float rotationAngle = getRotationAngle();

  int globalGlyphIndex = 0;
  for (TextRun& run: mTextView->getLayout().getTextFrame().getRuns()) {
    run.recalculatePixelRects(pixelsPerUnit,
                              rotationAngle,
                              *hint,
                              getEmboldenAmount(),
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

void
TextViewRenderer::buildGlyphs()
{
  shared_ptr<GlyphStore> glyphStore = mRenderContext->getGlyphStore();
  float pixelsPerUnit = getPixelsPerUnit();
  float rotationAngle = getRotationAngle();
  Vector4 textBounds = mTextView->getLayout().getTextFrame().bounds();
  std::shared_ptr<Hint> hint = createHint();

  // Only build glyphs in view.
  Vector2 translation = mCameraTranslation;;
  Vector4 canvasRect = Vector4::Create(
    -translation[0],
    -translation[1],
    -translation[0] + mCameraViewSize.x,
    -translation[1] + mCameraViewSize.y
  );

  std::vector<AtlasGlyph> atlasGlyphs;
  for (TextRun& run: mTextView->getLayout().getTextFrame().getRuns()) {
      for (int glyphIndex = 0; glyphIndex < run.getGlyphIDs().size(); glyphIndex++) {
          const Vector4 pixelRect = run.pixelRectForGlyphAt(glyphIndex);
          if (!rectsIntersect(pixelRect, canvasRect)) {
            continue;
          }

          int glyphID = run.getGlyphIDs()[glyphIndex];
          int glyphStoreIndex = glyphStore->indexOfGlyphWithID(glyphID);
          if (glyphStoreIndex == 0) {
            continue;
          }

          int subpixel = run.subpixelForGlyphAt(glyphIndex,
                                                pixelsPerUnit,
                                                rotationAngle,
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
TextViewRenderer::setGlyphTexCoords()
{
  float pixelsPerUnit = getPixelsPerUnit();
  float rotationAngle = getRotationAngle();

  Vector4 textBounds = mTextView->getLayout().getTextFrame().bounds();
  vector<AtlasGlyph> atlasGlyphs = mRenderContext->getAtlasGlyphs();

  shared_ptr<Hint> hint = createHint();

  mGlyphBounds.reserve(mTextView->getLayout().getTextFrame().totalGlyphCount() * 8);

  int globalGlyphIndex = 0;
  for (TextRun& run: mTextView->getLayout().getTextFrame().getRuns()) {
    for (int glyphIndex = 0;
       glyphIndex < run.getGlyphIDs().size();
       glyphIndex++, globalGlyphIndex++) {
      int textGlyphID = run.getGlyphIDs()[glyphIndex];

      int subpixel = run.subpixelForGlyphAt(glyphIndex,
                                            pixelsPerUnit,
                                            rotationAngle,
                                            *hint,
                                            SUBPIXEL_GRANULARITY,
                                            textBounds);
      // TODO(kearwood) - This is a hack!  Sometimes hasSubpixel should be true...
      bool hasSubpixel = false;  
      GlyphKey glyphKey(textGlyphID, hasSubpixel, subpixel);
      int sortKey = glyphKey.getSortKey();

      // Find index of glyphKey in atlasGlyphs, assuming atlasGlyphs is sorted by sortkey
      // TODO(kearwood) - This is slow...
      AtlasGlyph* atlasGlyph = nullptr;
      for(AtlasGlyph& g: atlasGlyphs) {
        if (g.getGlyphKey().getSortKey() == sortKey) {
          atlasGlyph = &g;
          break;
        }
      }
      // TODO(kearwood) - Error handling
      assert(atlasGlyph);
      // Set texture coordinates.
      const FT_BBox& atlasGlyphMetrics = mRenderContext->getFont()->metricsForGlyph(atlasGlyph->getGlyphKey().getID());

      UnitMetrics atlasGlyphUnitMetrics(atlasGlyphMetrics,
                                        rotationAngle,
                                        getEmboldenAmount());

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
TextViewRenderer::compositeIfNecessary()
{
  // Set up composite state.
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, mCameraViewSize.x, mCameraViewSize.y);
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
    blitProgram = mRenderContext->shaderPrograms()[shader_blitLinear];
    break;
  case gcm_on:
    blitProgram = mRenderContext->shaderPrograms()[shader_blitGamma];
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
    2.0 / mCameraViewSize.x,
    2.0 / mCameraViewSize.y,
    1.0
  );
  transform.translate(
    mCameraTranslation[0],
    mCameraTranslation[1],
    0.0f
  );

  // Blit.
  glUniformMatrix4fv(blitProgram->getUniforms()["uTransform"], 1, GL_FALSE, transform.c);
  glActiveTexture(GL_TEXTURE0);
  GLuint destTexture = mRenderContext->getAtlas()->ensureTexture(*mRenderContext);
  glBindTexture(GL_TEXTURE_2D, destTexture);
  glUniform1i(blitProgram->getUniforms()["uSource"], 0);
  glUniform2f(blitProgram->getUniforms()["uTexScale"], 1.0, 1.0);
  bindGammaLUT(Vector3::Create(1.0f, 1.0f, 1.0f), 1, blitProgram->getUniforms());
  int totalGlyphCount = mTextView->getLayout().getTextFrame().totalGlyphCount();
  glDrawElements(GL_TRIANGLES, totalGlyphCount * 6, GL_UNSIGNED_INT, 0);
}

} // namespace pathfinder
