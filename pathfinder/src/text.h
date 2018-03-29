// pathfinder/src/text.h
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.


#ifndef PATHFINDER_TEXT_H
#define PATHFINDER_TEXT_H

#include "platform.h"
#include "meshes.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <kraken-math.h>
#include <string>
#include <map>

namespace pathfinder {

const float SQRT_2 = sqrt(2.0f);

// Should match macOS 10.13 High Sierra.
//
// We multiply by sqrt(2) to compensate for the fact that dilation amounts are relative to the
// pixel square on macOS and relative to the vertex normal in Pathfinder.
const kraken::Vector2 STEM_DARKENING_FACTORS = kraken::Vector2::Create(
  0.0121f * SQRT_2,
  0.0121f * 1.25f * SQRT_2
);

// Likewise.
const kraken::Vector2 MAX_STEM_DARKENING_AMOUNT = kraken::Vector2::Create(
  0.3f * SQRT_2,
  0.3f * SQRT_2
);

// This value is a subjective cutoff. Above this ppem value, no stem darkening is performed.
float MAX_STEM_DARKENING_PIXELS_PER_EM = 72.0f;

class Hint;

class ExpandedMeshData
{
public:
  PathfinderPackedMeshes meshes;
};

struct PartitionResult
{
  PathfinderMeshPack meshes;
  float time;
};

struct PixelMetrics
{
  float left;
  float right;
  float ascent;
  float descent;
};

class PathfinderFont
{
public:
  PathfinderFont();
  bool load(FT_Library aLibrary, const __uint8_t* aData, size_t aDataLength,
            const std::string& aBuiltinFontName);
  std::string& getBuiltinFontName();

  FT_BBox& metricsForGlyph(int glyphID);
  FT_Face getFreeTypeFont();
private:
  FT_Face mFace;
  std::string mBuiltinFontName;
  std::map<int, FT_BBox> mMetricsCache;
}; // class PathfinderFont

class UnitMetrics
{
public:
  UnitMetrics(FT_BBox& metrics, float rotationAngle, kraken::Vector2& emboldenAmount);

  float mLeft;
  float mRight;
  float mAscent;
  float mDescent;
}; // class UnitMetrics

class TextRun
{
public:
  TextRun::TextRun(const std::string& aText,
                   kraken::Vector2 aOrigin,
                   std::shared_ptr<PathfinderFont> aFont);
  std::vector<int>& getGlyphIDs() const;
  std::vector<int>& getAdvances();
  kraken::Vector2 getOrigin();
  std::shared_ptr<PathfinderFont> getFont();
  void layout();
  kraken::Vector2 calculatePixelOriginForGlyphAt(int index,
                                                 float pixelsPerUnit,
                                                 float rotationAngle,
                                                 Hint hint,
                                                 kraken::Vector4 textFrameBounds);
  kraken::Vector4 pixelRectForGlyphAt(int index);
  int subpixelForGlyphAt(int index,
                         float pixelsPerUnit,
                         float rotationAngle,
                         Hint hint,
                         float subpixelGranularity,
                         kraken::Vector4 textFrameBounds);
  void recalculatePixelRects(float pixelsPerUnit,
                             float rotationAngle,
                             Hint hint,
                             kraken::Vector2 emboldenAmount,
                             float subpixelGranularity,
                             kraken::Vector4 textFrameBounds);
  float measure() const;
private:
  std::vector<int> mGlyphIDs;
  std::vector<int> mAdvances;
  kraken::Vector2 mOrigin;

  std::shared_ptr<PathfinderFont> mFont;
  std::vector<kraken::Vector4> mPixelRects;

}; // class TextRun

class TextFrame
{
public:
  TextFrame(const std::vector<TextRun>& aRuns, std::shared_ptr<PathfinderFont> aFont);
  const std::vector<TextRun>& getRuns() const;
  kraken::Vector3 getOrigin() const;
  ExpandedMeshData expandMeshes(const PathfinderMeshPack& meshes, std::vector<int>& glyphIDs);
  kraken::Vector4 bounds();
  int totalGlyphCount() const;
  std::vector<int> allGlyphIDs() const;
private:
  std::vector<TextRun> mRuns;
  kraken::Vector3 mOrigin;
  std::shared_ptr<PathfinderFont> mFont;
};

/*

/// Stores one copy of each glyph.
export class GlyphStore {
    readonly font: PathfinderFont;
    readonly glyphIDs: number[];

    constructor(font: PathfinderFont, glyphIDs: number[]) {
        this.font = font;
        this.glyphIDs = glyphIDs;
    }

    partition(): Promise<PartitionResult> {
        // Build the partitioning request to the server.
        let fontFace;
        if (this.font.builtinFontName != null)
            fontFace = { Builtin: this.font.builtinFontName };
        else
            fontFace = { Custom: base64js.fromByteArray(new Uint8Array(this.font.data)) };

        const request = {
            face: fontFace,
            fontIndex: 0,
            glyphs: this.glyphIDs.map(id => ({ id: id, transform: [1, 0, 0, 1, 0, 0] })),
            pointSize: this.font.opentypeFont.unitsPerEm,
        };

        // Make the request.
        let time = 0;
        // const PARTITION_FONT_ENDPOINT_URI: string = "/partition-font";
        return window.fetch(PARTITION_FONT_ENDPOINT_URI, {
            body: JSON.stringify(request),
            headers: {'Content-Type': 'application/json'} as any,
            method: 'POST',
        }).then(response => {
            time = parseServerTiming(response.headers);
            return response.arrayBuffer();
        }).then(buffer => {
            return {
                meshes: new PathfinderMeshPack(buffer),
                time: time,
            };
        });
    }

    indexOfGlyphWithID(glyphID: number): number | null {
        const index = _.sortedIndexOf(this.glyphIDs, glyphID);
        return index >= 0 ? index : null;
    }
}

export class SimpleTextLayout {
    readonly textFrame: TextFrame;

    constructor(font: PathfinderFont, text: string) {
        const lineHeight = font.opentypeFont.lineHeight();
        const textRuns: TextRun[] = text.split("\n").map((line, lineNumber) => {
            return new TextRun(line, [0.0, -lineHeight * lineNumber], font);
        });
        this.textFrame = new TextFrame(textRuns, font);
    }

    layoutRuns() {
        this.textFrame.runs.forEach(textRun => textRun.layout());
    }
}
*/

class Hint
{
public:
  Hint(const PathfinderFont& aFont, float aPixelsPerUnit, bool aUseHinting);
  float getXHeight() const;
  float getHintedXHeight() const;
  float getStemHeight() const;
  float getHintedStemHeight() const;
  kraken::Vector2 hintPosition(kraken::Vector2 position);
private:
  float mXHeight;
  float mHintedXHeight;
  float mStemHeight;
  float mHintedStemHeight;
  bool mUseHinting;
}; // class Hint



float calculatePixelXMin(UnitMetrics& metrics, float pixelsPerUnit);
float calculatePixelDescent(UnitMetrics& metrics, float pixelsPerUnit);
PixelMetrics calculateSubpixelMetricsForGlyph(UnitMetrics& metrics, float pixelsPerUnit, Hint hint);
kraken::Vector4 calculatePixelRectForGlyph(const UnitMetrics& metrics,
                                           kraken::Vector2 subpixelOrigin,
                                           float pixelsPerUnit,
                                           Hint hint);
kraken::Vector2 computeStemDarkeningAmount(float pixelsPerEm, float pixelsPerUnit);

/*

opentype.Font.prototype.lineHeight = function() {
const os2Table = this.tables.os2;
return os2Table.sTypoAscender - os2Table.sTypoDescender + os2Table.sTypoLineGap;
};

*/

} // namespace pathfinder

#endif // PATHFINDER_TEXT_H
