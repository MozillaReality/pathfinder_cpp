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

const float MIN_SCALE = 0.0025f;
const float MAX_SCALE = 0.5f;

TextRenderer::TextRenderer(std::shared_ptr<TextRenderContext> aRenderContext)
  : Renderer(aRenderContext)
  , mAtlasFramebuffer(0)
  , mAtlasDepthTexture(0)
{
  // TODO(kearwood) - Implement OrthographicCamera and uncomment:
  /*
        this.camera = new OrthographicCamera(this.renderContext.cameraView, {
            maxScale: MAX_SCALE,
            minScale: MIN_SCALE,
        });
  */
}

bool
TextRenderer::getIsMulticolor() const
{
  return false;
}

bool
TextRenderer::getNeedsStencil() const
{
  return mRenderContext->getFontSize() <= MAX_STEM_DARKENING_PIXELS_PER_EM;
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
  return mRenderContext->getAtlas()->getUsedSize();
}

kraken::Vector2
TextRenderer::getEmboldenAmount() const {
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
  return 0.0f;
}

kraken::Vector2
TextRenderer::getExtraEmboldenAmount() const {
  return Vector2::Zero();
}

float
TextRenderer::getPixelsPerUnit() const
{
  return 
  mRenderContext->getFontSize() / mRenderContext->getFont()->getFreeTypeFont()->units_per_EM;
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
    return computeStemDarkeningAmount(mRenderContext->getFontSize(), getPixelsPerUnit());
  }
  return Vector2::Zero();
}

kraken::Vector2
TextRenderer::getUsedSizeFactor() const
{
  Vector2i usedSize = mRenderContext->getAtlas()->getUsedSize();
  return Vector2::Create(usedSize.x / ATLAS_SIZE.x, usedSize.y / ATLAS_SIZE.y);
}

int
TextRenderer::getPathCount()
{
  return mRenderContext->getGlyphStore()->getGlyphIDs().size() * SUBPIXEL_GRANULARITY;
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

  for (const AtlasGlyph& glyph: mRenderContext->getAtlasGlyphs()) {
    const FT_BBox& atlasGlyphMetrics = mRenderContext->getFont()->metricsForGlyph(glyph.getGlyphKey().getID());
    // TODO(kearwood) error handling needed if FT_Bbox could not be populated?  Origin code "continue"'ed
    UnitMetrics atlasUnitMetrics(atlasGlyphMetrics, 0.0f, getEmboldenAmount());

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
  GLuint atlasColorTexture = mRenderContext->getAtlas()->ensureTexture(*mRenderContext);
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

  mRenderContext->setAtlasGlyphs(aAtlasGlyphs);
  mRenderContext->getAtlas()->layoutGlyphs(aAtlasGlyphs,
                                   *(mRenderContext->getFont()),
                                   getPixelsPerUnit(),
                                   getRotationAngle(),
                                   *createHint(),
                                   getEmboldenAmount());

  uploadPathTransforms(1);
  uploadPathColors(1);
}


std::vector<__uint8_t>
TextRenderer::pathColorsForObject(int objectIndex)
{
  int pathCount = getPathCount();

  std::vector<__uint8_t> pathColors;
  pathColors.reserve(4 * (pathCount + 1));

  for (int pathIndex = 0; pathIndex < pathCount; pathIndex++) {
    for (int channel = 0; channel < 3; channel++) {
      pathColors[(pathIndex + 1) * 4 + channel] = 0xff; // RGB
    }
    pathColors[(pathIndex + 1) * 4 + 3] = 0xff;         // alpha
  }

  return move(pathColors);
}

std::shared_ptr<PathTransformBuffers<std::vector<float>>>
TextRenderer::pathTransformsForObject(int objectIndex)
{
  int pathCount = getPathCount();
  int pixelsPerUnit = getPixelsPerUnit();
  float rotationAngle = getRotationAngle();

  // FIXME(pcwalton): This is a hack that tries to preserve the vertical extents of the glyph
  // after stem darkening. It's better than nothing, but we should really do better.
  //
  // This hack seems to produce *better* results than what macOS does on sans-serif fonts;
  // the ascenders and x-heights of the glyphs are pixel snapped, while they aren't on macOS.
  // But we should really figure out what macOS does…
  const FT_Face& fontFace = mRenderContext->getFont()->getFreeTypeFont();
  float ascender = fontFace->ascender;
  Vector2 emboldenAmount = getEmboldenAmount();
  float stemDarkeningYScale = (ascender + emboldenAmount[1]) / ascender;

  Vector2 stemDarkeningOffset = emboldenAmount;
  stemDarkeningOffset *= pixelsPerUnit;
  stemDarkeningOffset *= SQRT_1_2;
  stemDarkeningOffset.y *= stemDarkeningYScale;

  Matrix2x3 transform = Matrix2x3::Identity();
  std::shared_ptr<PathTransformBuffers<std::vector<float>>> transforms
    = createPathTransformBuffers(pathCount);

  for (const AtlasGlyph& glyph: mRenderContext->getAtlasGlyphs()) {
    int pathID = glyph.getPathID();
    Vector2 atlasOrigin = glyph.calculateSubpixelOrigin(pixelsPerUnit);

    transform = Matrix2x3::Identity();
    transform.translate(atlasOrigin);
    transform.translate(stemDarkeningOffset);
    transform.rotate(rotationAngle);
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
  return make_shared<Hint>(*mRenderContext->getFont(),
                           getPixelsPerUnit(),
                           mRenderContext->getUseHinting());
}

ShaderID
TextRenderer::getDirectCurveProgramName()
{
  return shader_directCurve;
}

ShaderID
TextRenderer::getDirectInteriorProgramName(DirectRenderingMode renderingMode)
{
  return shader_directInterior;
}

} // namespace pathfinder
