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

class PartitionResult
{
public:
  PathfinderMeshPack meshes;
  float time;
};

class PixelMetrics
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
  std::vector<int>& getGlyphIDs();
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
  float measure();
private:
  std::vector<int> mGlyphIDs;
  std::vector<int> mAdvances;
  kraken::Vector2 mOrigin;

  std::shared_ptr<PathfinderFont> mFont;
  std::vector<kraken::Vector4> mPixelRects;

}; // class TextRun

/*
export class TextFrame {
    readonly runs: TextRun[];
    readonly origin: glmatrix.vec3;

    private readonly font: PathfinderFont;

    constructor(runs: TextRun[], font: PathfinderFont) {
        this.runs = runs;
        this.origin = glmatrix.vec3.create();
        this.font = font;
    }

    expandMeshes(meshes: PathfinderMeshPack, glyphIDs: number[]): ExpandedMeshData {
        const pathIDs = [];
        for (const textRun of this.runs) {
            for (const glyphID of textRun.glyphIDs) {
                if (glyphID === 0)
                    continue;
                const pathID = _.sortedIndexOf(glyphIDs, glyphID);
                pathIDs.push(pathID + 1);
            }
        }

        return {
            meshes: new PathfinderPackedMeshes(meshes, pathIDs),
        };
    }

    get bounds(): glmatrix.vec4 {
        if (this.runs.length === 0)
            return glmatrix.vec4.create();

        const upperLeft = glmatrix.vec2.clone(this.runs[0].origin);
        const lowerRight = glmatrix.vec2.clone(_.last(this.runs)!.origin);

        const lowerLeft = glmatrix.vec2.clone([upperLeft[0], lowerRight[1]]);
        const upperRight = glmatrix.vec2.clone([lowerRight[0], upperLeft[1]]);

        const lineHeight = this.font.opentypeFont.lineHeight();
        lowerLeft[1] -= lineHeight;
        upperRight[1] += lineHeight * 2.0;

        upperRight[0] = _.defaultTo<number>(_.max(this.runs.map(run => run.measure)), 0.0);

        return glmatrix.vec4.clone([lowerLeft[0], lowerLeft[1], upperRight[0], upperRight[1]]);
    }

    get totalGlyphCount(): number {
        return _.sumBy(this.runs, run => run.glyphIDs.length);
    }

    get allGlyphIDs(): number[] {
        const glyphIDs = [];
        for (const run of this.runs)
            glyphIDs.push(...run.glyphIDs);
        return glyphIDs;
    }
}

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

export class Hint {
    readonly xHeight: number;
    readonly hintedXHeight: number;
    readonly stemHeight: number;
    readonly hintedStemHeight: number;

    private useHinting: boolean;

    constructor(font: PathfinderFont, pixelsPerUnit: number, useHinting: boolean) {
        this.useHinting = useHinting;

        const os2Table = font.opentypeFont.tables.os2;
        this.xHeight = os2Table.sxHeight != null ? os2Table.sxHeight : 0;
        this.stemHeight = os2Table.sCapHeight != null ? os2Table.sCapHeight : 0;

        if (!useHinting) {
            this.hintedXHeight = this.xHeight;
            this.hintedStemHeight = this.stemHeight;
        } else {
            this.hintedXHeight = Math.round(Math.round(this.xHeight * pixelsPerUnit) /
                                            pixelsPerUnit);
            this.hintedStemHeight = Math.round(Math.round(this.stemHeight * pixelsPerUnit) /
                                               pixelsPerUnit);
        }
    }

    /// NB: This must match `hintPosition()` in `common.inc.glsl`.
    hintPosition(position: glmatrix.vec2): glmatrix.vec2 {
        if (!this.useHinting)
            return position;

        if (position[1] >= this.stemHeight) {
            const y = position[1] - this.stemHeight + this.hintedStemHeight;
            return glmatrix.vec2.clone([position[0], y]);
        }

        if (position[1] >= this.xHeight) {
            const y = lerp(this.hintedXHeight, this.hintedStemHeight,
                           (position[1] - this.xHeight) / (this.stemHeight - this.xHeight));
            return glmatrix.vec2.clone([position[0], y]);
        }

        if (position[1] >= 0.0) {
            const y = lerp(0.0, this.hintedXHeight, position[1] / this.xHeight);
            return glmatrix.vec2.clone([position[0], y]);
        }

        return position;
    }
}

export function calculatePixelXMin(metrics: UnitMetrics, pixelsPerUnit: number): number {
    return Math.floor(metrics.left * pixelsPerUnit);
}

export function calculatePixelDescent(metrics: UnitMetrics, pixelsPerUnit: number): number {
    return Math.floor(metrics.descent * pixelsPerUnit);
}

function calculateSubpixelMetricsForGlyph(metrics: UnitMetrics, pixelsPerUnit: number, hint: Hint):
                                          PixelMetrics {
    const ascent = hint.hintPosition(glmatrix.vec2.fromValues(0, metrics.ascent))[1];
    return {
        ascent: ascent * pixelsPerUnit,
        descent: metrics.descent * pixelsPerUnit,
        left: metrics.left * pixelsPerUnit,
        right: metrics.right * pixelsPerUnit,
    };
}

export function calculatePixelRectForGlyph(metrics: UnitMetrics,
                                           subpixelOrigin: glmatrix.vec2,
                                           pixelsPerUnit: number,
                                           hint: Hint):
                                           glmatrix.vec4 {
    const pixelMetrics = calculateSubpixelMetricsForGlyph(metrics, pixelsPerUnit, hint);
    return glmatrix.vec4.clone([Math.floor(subpixelOrigin[0] + pixelMetrics.left),
                                Math.floor(subpixelOrigin[1] + pixelMetrics.descent),
                                Math.ceil(subpixelOrigin[0] + pixelMetrics.right),
                                Math.ceil(subpixelOrigin[1] + pixelMetrics.ascent)]);
}

export function computeStemDarkeningAmount(pixelsPerEm: number, pixelsPerUnit: number):
                                           glmatrix.vec2 {
    const amount = glmatrix.vec2.create();
    if (pixelsPerEm > MAX_STEM_DARKENING_PIXELS_PER_EM)
        return amount;

    glmatrix.vec2.scale(amount, STEM_DARKENING_FACTORS, pixelsPerEm);
    glmatrix.vec2.min(amount, amount, MAX_STEM_DARKENING_AMOUNT);
    glmatrix.vec2.scale(amount, amount, 1.0 / pixelsPerUnit);
    return amount;
}

opentype.Font.prototype.lineHeight = function() {
    const os2Table = this.tables.os2;
    return os2Table.sTypoAscender - os2Table.sTypoDescender + os2Table.sTypoLineGap;
};

*/

};

#endif // PATHFINDER_TEXT_H
