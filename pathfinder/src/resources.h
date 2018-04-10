// pathfinder/src/resources.h
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
#ifndef PATHFINDER_RESOURCES_H
#define PATHFINDER_RESOURCES_H

namespace pathfinder {

#include "resources/partition_font.h"

const char* const shader_common =
#include "resources/shaders/gl410/common.inc.glsl"
;

const char* const shader_blit_gamma_fs =
#include "resources/shaders/gl410/blit-gamma.fs.glsl"
;

const char* const shader_blit_linear_fs =
#include "resources/shaders/gl410/blit-linear.fs.glsl"
;

const char* const shader_blit_vs =
#include "resources/shaders/gl410/blit.vs.glsl"
;

const char* const shader_conservative_interior_vs =
#include "resources/shaders/gl410/conservative-interior.vs.glsl"
;

const char* const shader_direct_3d_curve_vs =
#include "resources/shaders/gl410/direct-3d-curve.vs.glsl"
;

const char* const shader_direct_3d_interior_vs =
#include "resources/shaders/gl410/direct-3d-interior.vs.glsl"
;

const char* const shader_direct_curve_fs =
#include "resources/shaders/gl410/direct-curve.fs.glsl"
;

const char* const shader_direct_curve_vs =
#include "resources/shaders/gl410/direct-curve.vs.glsl"
;

const char* const shader_direct_interior_fs =
#include "resources/shaders/gl410/direct-interior.fs.glsl"
;

const char* const shader_direct_interior_vs =
#include "resources/shaders/gl410/direct-interior.vs.glsl"
;

const char* const shader_mcaa_fs =
#include "resources/shaders/gl410/mcaa.fs.glsl"
;

const char* const shader_mcaa_vs =
#include "resources/shaders/gl410/mcaa.vs.glsl"
;

const char* const shader_ssaa_subpixel_resolve_fs =
#include "resources/shaders/gl410/ssaa-subpixel-resolve.fs.glsl"
;

const char* const shader_stencil_aaa_fs =
#include "resources/shaders/gl410/stencil-aaa.fs.glsl"
;

const char* const shader_stencil_aaa_vs =
#include "resources/shaders/gl410/stencil-aaa.vs.glsl"
;

const char* const shader_xcaa_mono_resolve_fs =
#include "resources/shaders/gl410/xcaa-mono-resolve.fs.glsl"
;

const char* const shader_xcaa_mono_resolve_vs =
#include "resources/shaders/gl410/xcaa-mono-resolve.vs.glsl"
;

const char* const shader_xcaa_mono_subpixel_resolve_fs =
#include "resources/shaders/gl410/xcaa-mono-subpixel-resolve.fs.glsl"
;

const char* const shader_xcaa_mono_subpixel_resolve_vs =
#include "resources/shaders/gl410/xcaa-mono-subpixel-resolve.vs.glsl"
;

} // namespace pathfinder

#endif // PATHFINDER_RESOURCES_H
