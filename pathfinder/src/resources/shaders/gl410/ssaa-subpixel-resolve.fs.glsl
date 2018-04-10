R"(
// pathfinder/shaders/gles2/ssaa-subpixel-resolve.fs.glsl
//
// Copyright (c) 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

//! Performs subpixel antialiasing for LCD screens by converting a
//! 3x-oversampled RGBA color buffer to an RGB framebuffer, applying the
//! FreeType color defringing filter as necessary.

precision mediump float;

/// The alpha coverage texture.
uniform sampler2D uSource;
/// The dimensions of the alpha coverage texture, in texels.
uniform ivec2 uSourceDimensions;

in vec2 vTexCoord;

out vec4 fragmentColor;

float sampleSource(float deltaX) {
    return texture(uSource, vec2(vTexCoord.x + deltaX, vTexCoord.y)).r;
}

void main() {
    float onePixel = 1.0 / float(uSourceDimensions.x);

    float shade0 = sampleSource(0.0);
    vec3 shadeL = vec3(sampleSource(-1.0 * onePixel),
                       sampleSource(-2.0 * onePixel),
                       sampleSource(-3.0 * onePixel));
    vec3 shadeR = vec3(sampleSource(1.0 * onePixel),
                       sampleSource(2.0 * onePixel),
                       sampleSource(3.0 * onePixel));

    vec3 color = vec3(lcdFilter(shadeL.z, shadeL.y, shadeL.x, shade0,   shadeR.x),
                      lcdFilter(shadeL.y, shadeL.x, shade0,   shadeR.x, shadeR.y),
                      lcdFilter(shadeL.x, shade0,   shadeR.x, shadeR.y, shadeR.z));

    fragmentColor = vec4(color, 1.0);
}
)"
