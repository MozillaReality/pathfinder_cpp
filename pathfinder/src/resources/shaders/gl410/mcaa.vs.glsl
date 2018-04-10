R"(
// pathfinder/shaders/gles2/mcaa.vs.glsl
//
// Copyright (c) 2018 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

//! Renders paths when performing *mesh coverage antialiasing* (MCAA). This
//! one shader handles both lines and curves.
//!
//! This shader expects to render to a standard RGB color buffer in the
//! multicolor case or a single-channel floating-point color buffer in the
//! monochrome case.
//!
//! Set state as follows depending on whether multiple overlapping multicolor
//! paths are present:
//!
//! * When paths of multiple colors are present, use
//!   `glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE)` and
//!   set `uMulticolor` to 1.
//!
//! * Otherwise, if only one color of path is present, use
//!   `glBlendFunc(GL_ONE, GL_ONE)` and set `uMulticolor` to 0.
//!
//! Use this shader only when your transform is only a scale and/or
//! translation, not a perspective, rotation, or skew. (Otherwise, consider
//! repartitioning the path to generate a new mesh, or, alternatively, use the
//! direct Loop-Blinn shaders.)

#define MAX_SLOPE   10.0

precision highp float;

/// A scale and transform to be applied to the object.
uniform vec4 uTransformST;
uniform vec2 uTransformExt;
/// The framebuffer size in pixels.
uniform ivec2 uFramebufferSize;
/// The size of the path transform buffer texture in texels.
uniform ivec2 uPathTransformSTDimensions;
/// The path transform buffer texture, one dilation per path ID.
uniform sampler2D uPathTransformST;
uniform ivec2 uPathTransformExtDimensions;
uniform sampler2D uPathTransformExt;
/// The size of the path colors buffer texture in texels.
uniform ivec2 uPathColorsDimensions;
/// The path colors buffer texture, one color per path ID.
uniform sampler2D uPathColors;
/// True if multiple colors are being rendered; false otherwise.
///
/// If this is true, then points will be snapped to the nearest pixel.
uniform bool uMulticolor;

in vec2 aTessCoord;
in vec4 aRect;
in vec4 aUV;
in vec4 aDUVDX;
in vec4 aDUVDY;
// TODO(pcwalton): This is redundant; sign 0 can be used to indicate lines.
in vec4 aSignMode;
in float aPathID;

out vec4 vColor;
out vec4 vUV;
out vec4 vSignMode;

void main() {
    vec2 tessCoord = aTessCoord;
    int pathID = int(floor(aPathID));

    vec4 color;
    if (uMulticolor)
        color = fetchFloat4Data(uPathColors, pathID, uPathColorsDimensions);
    else
        color = vec4(1.0);

    vec2 transformExt;
    vec4 transformST = fetchPathAffineTransform(transformExt,
                                                uPathTransformST,
                                                uPathTransformSTDimensions,
                                                uPathTransformExt,
                                                uPathTransformExtDimensions,
                                                pathID);

    mat2 globalTransformLinear = mat2(uTransformST.x, uTransformExt, uTransformST.y);
    mat2 localTransformLinear = mat2(transformST.x, -transformExt, transformST.y);
    mat2 rectTransformLinear = mat2(aRect.z - aRect.x, 0.0, 0.0, aRect.w - aRect.y);
    mat2 transformLinear = globalTransformLinear * localTransformLinear * rectTransformLinear;

    vec2 translation = transformST.zw + localTransformLinear * aRect.xy;
    translation = uTransformST.zw + globalTransformLinear * translation;

    float onePixel = 2.0 / float(uFramebufferSize.y);
    float dilation = length(transformVertexPositionInverseLinear(vec2(0.0, onePixel),
                                                                 transformLinear));
    tessCoord.y += tessCoord.y < 0.5 ? -dilation : dilation;

    vec2 position = transformLinear * tessCoord + translation;
    vec4 uv = aUV + tessCoord.x * aDUVDX + tessCoord.y * aDUVDY;
    float depth = convertPathIndexToViewportDepthValue(pathID);

    gl_Position = vec4(position, depth, 1.0);
    vColor = color;
    vUV = uv;
    vSignMode = aSignMode;
}
)"
