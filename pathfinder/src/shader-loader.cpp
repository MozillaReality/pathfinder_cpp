// pathfinder/src/shader-loader.cpp
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.
#include "shader-loader.h"

#include <assert.h>

using namespace std;

namespace pathfinder {

PathfinderShaderProgram::PathfinderShaderProgram(const std::string& aProgramName,
                                                 const UnlinkedShaderProgram& aUnlinkedShaderProgram)
  : mProgram(0)
  , mProgramName(aProgramName)
{
  // TODO(kearwood) Error handling
  GLDEBUG(mProgram = glCreateProgram());
  GLDEBUG(glAttachShader(mProgram, aUnlinkedShaderProgram.vertex));
  GLDEBUG(glAttachShader(mProgram, aUnlinkedShaderProgram.fragment));
  GLDEBUG(glLinkProgram(mProgram));

  // Report any compile issues to stderr
  // TODO(kearwood) - We should move this to a non-CTOR function and
  // return success/fail bool to caller
  GLint link_success = GL_FALSE;
  GLDEBUG(glGetProgramiv(mProgram, GL_LINK_STATUS, &link_success));
  
  if(link_success != GL_TRUE) {
    // Report any linking issues to stderr
    fprintf(stderr, "Failed to link shader program: %s", aProgramName.c_str());
    GLsizei logLength = 0; // In case glGetProgramiv fails
    GLDEBUG(glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &logLength));
    if (logLength > 0) {
      GLchar *log = (GLchar *)malloc(logLength + 1);
      assert(log != NULL);
      log[0] = '\0'; // In case glGetProgramInfoLog fails
      GLDEBUG(glGetProgramInfoLog(mProgram, logLength, &logLength, log));
      log[logLength] = '\0';
      fprintf(stderr, "Program link log:\n%s", log);
      free(log);
    }
    GLDEBUG(glDeleteProgram(mProgram));
    mProgram = 0;
  }

  GLint uniformCount = 0;
  GLDEBUG(glGetProgramiv(mProgram, GL_ACTIVE_UNIFORMS, &uniformCount));
  GLint attributeCount = 0;
  GLDEBUG(glGetProgramiv(mProgram, GL_ACTIVE_ATTRIBUTES, &attributeCount));


  for (GLint uniformIndex = 0; uniformIndex < uniformCount; uniformIndex++) {
    const GLsizei kMaxUniformNameLength = 128;  
    GLint uniformSize = 0;
    GLenum uniformType;
    char uniformName[GL_ACTIVE_UNIFORM_MAX_LENGTH];
    uniformName[0] = '\0'; // in case glGetActiveUniform fails
    GLint uniformLocation = 0;
    GLDEBUG(glGetActiveUniform(mProgram,
                               uniformIndex,
                               GL_ACTIVE_UNIFORM_MAX_LENGTH,
                               NULL,
                               &uniformSize,
                               &uniformType,
                               uniformName));
    GLDEBUG(uniformLocation = glGetUniformLocation(mProgram, uniformName));
    mUniforms[uniformName] = uniformLocation;
  }
  for (GLint attributeIndex = 0; attributeIndex < attributeCount; attributeIndex++) {
    GLint attributeSize = 0;
    GLenum attributeType;
    char attributeName[GL_ACTIVE_ATTRIBUTE_MAX_LENGTH];
    attributeName[0] = '\0'; // in case glGetActiveAttribute fails
    glGetActiveAttrib(mProgram, attributeIndex, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, NULL, &attributeSize, &attributeType, attributeName);
    mAttributes[attributeName] = attributeIndex;
  }
}

} // namepsace pathfinder
