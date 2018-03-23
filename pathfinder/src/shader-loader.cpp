#include "shader-loader.h"

#include <assert.h>

using namespace std;

namespace pathfinder {

PathfinderShaderProgram::PathfinderShaderProgram(const std::string& aProgramName,
                                                 const UnlinkedShaderProgram& aUnlinkedShaderProgram)
  : mProgram(0)
  , mProgramName(aProgramName)
{
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


  for (int uniformIndex = 0; uniformIndex < uniformCount; uniformIndex++) {
    const GLsizei kMaxUniformNameLength = 128;  
    GLint uniformSize = 0;
    GLenum uniformType;
    char uniformName[kMaxUniformNameLength];
    uniformName[0] = '\0'; // in case glGetActiveUniform fails
    GLint uniformLocation = 0;

    GLDEBUG(glGetActiveUniform(mProgram,
                               uniformIndex,
                               kMaxUniformNameLength,
                               &uniformSize,
                               uniformName));
    GLDEBUG(uniformLocation = glGetUniformLocation(mProgram, uniformName));
    mUniforms[uniformName] = uniformLocation;
    // TODO(kearwood) Error handling
  }
  for (int attributeIndex = 0; attributeIndex < attributeCount; attributeIndex++) {
      const attributeName = unwrapNull(gl.getActiveAttrib(mProgram, attributeIndex)).name;
      mAttributes[attributeName] = attributeIndex;
  }
}

} // namepsace pathfinder
