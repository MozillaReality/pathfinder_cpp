#ifndef PATHFINDER_PLATFORM_H
#define PATHFINDER_PLATFORM_H

#if defined(_WIN32) || defined(_WIN64)

// #include <GL/gl.h>
// #include <GL/glu.h>
#include <glad/glad.h>

typedef __int32 __int32_t;
typedef unsigned __int32 __uint32_t;
typedef __int16 __int16_t;
typedef unsigned __int16 __uint16_t;
typedef __int8 __int8_t;
typedef unsigned __int8 __uint8_t;

#elif __APPLE__
#  include "TargetConditionals.h"
#  if (TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR) || TARGET_OS_IPHONE
#    include <OpenGLES/ES2/gl.h>
#    include <OpenGLES/ES2/glext.h>
#  else
#    include <OpenGL/gl.h>
#    include <OpenGL/glu.h>
#    include <OpenGL/glext.h>
#  endif
#elif defined(__ANDROID__) || defined(ANDROID)
#  include <GLES2/gl2.h>
#  include <GLES2/gl2ext.h>
#elif defined(__linux__) || defined(__unix__) || defined(__posix__)
#  include <GL/gl.h>
#  include <GL/glu.h>
#  include <GL/glext.h>
#else
#  error platform not supported.
#endif


#endif
