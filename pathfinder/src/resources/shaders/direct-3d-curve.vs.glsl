R"(
// pathfinder/shaders/gles2/direct-3d-curve.vs.glsl
//
// Copyright (c) 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

//! A version of `direct-curve` that takes each vertex's Z value from the
//! transform instead of the path ID.
//!
//! FIXME(pcwalton): For CSS 3D transforms, I think `direct-curve` will need
//! to do what this shader does. Perhaps these two shaders should be unified…

precision highp float;

/// A 3D transform to be applied to all points.
uniform mat4 uTransform;
/// The size of the path transform buffer texture in texels.
uniform ivec2 uPathTransformSTDimensions;
/// The path transform buffer texture, one dilation per path ID.
uniform sampler2D uPathTransformST;
/// The size of the extra path transform factors buffer texture in texels.
uniform ivec2 uPathTransformExtDimensions;
/// The extra path transform factors buffer texture, packed two path transforms per texel.
uniform sampler2D uPathTransformExt;
/// The size of the path colors buffer texture in texels.
uniform ivec2 uPathColorsDimensions;
/// The path colors buffer texture, one color per path ID.
uniform sampler2D uPathColors;
/// The amount of faux-bold to apply, in local path units.
uniform vec2 uEmboldenAmount;

/// The 2D position of this point.
attribute vec2 aPosition;
/// The angle of the 2D normal for this point.
attribute float aNormalAngle;
/// The vertex ID. In OpenGL 3.0+, this can be omitted in favor of `gl_VertexID`.
attribute float aVertexID;
/// The path ID, starting from 1.
attribute float aPathID;

/// The fill color of this path.
varying vec4 vColor;
/// The outgoing abstract Loop-Blinn texture coordinate.
varying vec2 vTexCoord;

void main() {
    int pathID = int(aPathID);
    int vertexID = int(aVertexID);

    vec2 pathTransformExt;
    vec4 pathTransformST = fetchPathAffineTransform(pathTransformExt,
                                                    uPathTransformST,
                                                    uPathTransformSTDimensions,
                                                    uPathTransformExt,
                                                    uPathTransformExtDimensions,
                                                    pathID);

    vec2 position = dilatePosition(aPosition, aNormalAngle, uEmboldenAmount);
    position = transformVertexPositionAffine(position, pathTransformST, pathTransformExt);

    gl_Position = uTransform * vec4(position, 0.0, 1.0);

    int vertexIndex = imod(vertexID, 3);
    vec2 texCoord = vec2(float(vertexIndex) * 0.5, float(vertexIndex == 2));

    vColor = fetchFloat4Data(uPathColors, pathID, uPathColorsDimensions);
    vTexCoord = texCoord;
}
)"
