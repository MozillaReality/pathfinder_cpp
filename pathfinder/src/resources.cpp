// pathfinder/src/resources.cpp
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include "resources.h"

#ifndef PATHFINDER_RESOURCES_H
#define PATHFINDER_RESOURCES_H

namespace pathfinder {

const char* shader_common =
#include "resources/shaders/common.inc.glsl"
;

const char* shader_blit_gamma_fs =
#include "resources/shaders/blit-gamma.fs.glsl"
;

const char* shader_blit_linear_fs =
#include "resources/shaders/blit-linear.fs.glsl"
;

const char* shader_blit_vs =
#include "resources/shaders/blit.vs.glsl"
;

const char* shader_conservative_interior_vs =
#include "resources/shaders/conservative-interior.vs.glsl"
;

const char* shader_direct_3d_curve_vs =
#include "resources/shaders/direct-3d-curve.vs.glsl"
;

const char* shader_direct_3d_interior_vs =
#include "resources/shaders/direct-3d-interior.vs.glsl"
;

const char* shader_direct_curve_fs =
#include "resources/shaders/direct-curve.fs.glsl"
;

const char* shader_direct_curve_vs =
#include "resources/shaders/direct-curve.vs.glsl"
;

const char* shader_direct_interior_fs =
#include "resources/shaders/direct-interior.fs.glsl"
;

const char* shader_direct_interior_vs =
#include "resources/shaders/direct-interior.vs.glsl"
;

const char* shader_mcaa_fs =
#include "resources/shaders/mcaa.fs.glsl"
;

const char* shader_mcaa_vs =
#include "resources/shaders/mcaa.vs.glsl"
;

const char* shader_ssaa_subpixel_resolve_fs =
#include "resources/shaders/ssaa-subpixel-resolve.fs.glsl"
;

const char* shader_stencil_aaa_fs =
#include "resources/shaders/stencil-aaa.fs.glsl"
;

const char* shader_stencil_aaa_vs =
#include "resources/shaders/stencil-aaa.vs.glsl"
;

const char* shader_xcaa_mono_resolve_fs =
#include "resources/shaders/xcaa-mono-resolve.fs.glsl"
;

const char* shader_xcaa_mono_resolve_vs =
#include "resources/shaders/xcaa-mono-resolve.vs.glsl"
;

const char* shader_xcaa_mono_subpixel_resolve_fs =
#include "resources/shaders/xcaa-mono-subpixel-resolve.fs.glsl"
;

const char* shader_xcaa_mono_subpixel_resolve_vs =
#include "resources/shaders/xcaa-mono-subpixel-resolve.vs.glsl"
;

} // namespace pathfinder

#endif // PATHFINDER_RESOURCES_H