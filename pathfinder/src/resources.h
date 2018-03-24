#ifndef PATHFINDER_RESOURCES_H
#define PATHFINDER_RESOURCES_H

namespace pathfinder {

#include "resources/eb_garamond_ttf.h"
#include "resources/partition_font.h"

const char* const shader_common =
#include "resources/shaders/common.inc.glsl"
;

const char* const shader_blit_gamma_fs =
#include "resources/shaders/blit-gamma.fs.glsl"
;

const char* const shader_blit_linear_fs =
#include "resources/shaders/blit-linear.fs.glsl"
;

const char* const shader_blit_vs =
#include "resources/shaders/blit.vs.glsl"
;

const char* const shader_conservative_interior_vs =
#include "resources/shaders/conservative-interior.vs.glsl"
;

const char* const shader_direct_3d_curve_vs =
#include "resources/shaders/direct-3d-curve.vs.glsl"
;

const char* const shader_direct_3d_interior_vs =
#include "resources/shaders/direct-3d-interior.vs.glsl"
;

const char* const shader_direct_curve_fs =
#include "resources/shaders/direct-curve.fs.glsl"
;

const char* const shader_direct_curve_vs =
#include "resources/shaders/direct-curve.vs.glsl"
;

const char* const shader_direct_interior_fs =
#include "resources/shaders/direct-interior.fs.glsl"
;

const char* const shader_direct_interior_vs =
#include "resources/shaders/direct-interior.vs.glsl"
;

const char* const shader_mcaa_fs =
#include "resources/shaders/mcaa.fs.glsl"
;

const char* const shader_mcaa_vs =
#include "resources/shaders/mcaa.vs.glsl"
;

const char* const shader_ssaa_subpixel_resolve_fs =
#include "resources/shaders/ssaa-subpixel-resolve.fs.glsl"
;

const char* const shader_stencil_aaa_fs =
#include "resources/shaders/stencil-aaa.fs.glsl"
;

const char* const shader_stencil_aaa_vs =
#include "resources/shaders/stencil-aaa.vs.glsl"
;

const char* const shader_xcaa_mono_resolve_fs =
#include "resources/shaders/xcaa-mono-resolve.fs.glsl"
;

const char* const shader_xcaa_mono_resolve_vs =
#include "resources/shaders/xcaa-mono-resolve.vs.glsl"
;

const char* const shader_xcaa_mono_subpixel_resolve_fs =
#include "resources/shaders/xcaa-mono-subpixel-resolve.fs.glsl"
;

const char* const shader_xcaa_mono_subpixel_resolve_vs =
#include "resources/shaders/xcaa-mono-subpixel-resolve.vs.glsl"
;

} // namespace pathfinder

#endif // PATHFINDER_RESOURCES_H