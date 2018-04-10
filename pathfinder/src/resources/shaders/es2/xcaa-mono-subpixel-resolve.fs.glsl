R"(
// pathfinder/shaders/gles2/xcaa-mono-subpixel-resolve.fs.glsl
//
// Copyright (c) 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

//! Performs subpixel antialiasing for LCD screens by converting a
//! 3x-oversampled single-channel color buffer to an RGB framebuffer, applying
//! the FreeType color defringing filter as necessary.

precision mediump float;

/// The background color of the monochrome path.
uniform vec4 uBGColor;
/// The foreground color of the monochrome path.
uniform vec4 uFGColor;
/// The alpha coverage texture.
uniform sampler2D uAAAlpha;
/// The dimensions of the alpha coverage texture, in texels.
uniform ivec2 uAAAlphaDimensions;

varying vec2 vTexCoord;

float sampleSource(float deltaX) {
    return abs(texture2D(uAAAlpha, vec2(vTexCoord.s + deltaX, vTexCoord.y)).r);
}

void main() {
    float onePixel = 1.0 / float(uAAAlphaDimensions.x);

    float shade0 = sampleSource(0.0);
    vec3 shadeL = vec3(sampleSource(-1.0 * onePixel),
                       sampleSource(-2.0 * onePixel),
                       sampleSource(-3.0 * onePixel));
    vec3 shadeR = vec3(sampleSource(1.0 * onePixel),
                       sampleSource(2.0 * onePixel),
                       sampleSource(3.0 * onePixel));

    vec3 shades = vec3(lcdFilter(shadeL.z, shadeL.y, shadeL.x, shade0,   shadeR.x),
                       lcdFilter(shadeL.y, shadeL.x, shade0,   shadeR.x, shadeR.y),
                       lcdFilter(shadeL.x, shade0,   shadeR.x, shadeR.y, shadeR.z));

    vec3 color = mix(uBGColor.rgb, uFGColor.rgb, shades);
    float alpha = any(greaterThan(shades, vec3(0.0))) ? uFGColor.a : uBGColor.a;
    gl_FragColor = alpha * vec4(color, 1.0);
}
)"
