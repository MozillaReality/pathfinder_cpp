R"(
// pathfinder/shaders/gles2/blit-linear.fs.glsl
//
// Copyright (c) 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

//! A trivial shader that does nothing more than blit a texture.

precision mediump float;

/// The source texture to blit.
uniform sampler2D uSource;

/// The incoming texture coordinate.
in vec2 vTexCoord;

out vec4 fragmentColor;

void main() {
    fragmentColor = texture(uSource, vTexCoord);
    fragmentColor = vec4(0.0, 0.0, 1.0, 1.0); // TOOD(kearwood) FINDME! KIP!! HACK!!
}
)"
