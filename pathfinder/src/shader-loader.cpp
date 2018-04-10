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

ShaderManager::ShaderManager()
{
  mPrograms.reserve(program_count);
}


ShaderManager::~ShaderManager()
{
}

GLuint
ShaderManager::loadShader(const char* aName, const char* aSource, GLenum aType)
{
  const char* shader_source[2];
  shader_source[0] = shader_common;
  shader_source[1] = aSource;

  GLuint shader = 0;

  GLDEBUG(shader = glCreateShader(GL_VERTEX_SHADER));
  GLDEBUG(glShaderSource(shader, 2, shader_source, NULL));
  GLDEBUG(glCompileShader(shader));

  // Report any compile issues to stderr
  GLint compile_success = GL_FALSE;
  GLDEBUG(glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_success));

  if (!compile_success) {
    // Report any compile issues to stderr
    fprintf(stderr, "Failed to compile %s shader: %s\n",
            aType == GL_VERTEX_SHADER ? "vertex": "fragment",
            aName);
    GLsizei logLength = 0; // In case glGetShaderiv fails
    GLDEBUG(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength));
    if (logLength > 0) {
      GLchar *log = (GLchar *)malloc(logLength + 1);
      assert(log != NULL);
      log[0] = '\0'; // In case glGetProgramInfoLog fails
      GLDEBUG(glGetShaderInfoLog(shader, logLength, &logLength, log));
      log[logLength] = '\0';
      fprintf(stderr, "Shader compile log:\n%s", log);
      free(log);
    }
    glDeleteShader(shader);
    shader = 0;
  }


  return shader;
}

bool
ShaderManager::init()
{
  bool succeeded = true;
  GLuint vertexShaders[vs_count];
  GLuint fragmentShaders[fs_count];
  memset(vertexShaders, 0, sizeof(vertexShaders));
  memset(fragmentShaders, 0, sizeof(fragmentShaders));

  for (int i = 0; i < vs_count && succeeded; i++) {
    vertexShaders[i] = loadShader(VertexShaderNames[i],
                                  VertexShaderSource[i],
                                  GL_VERTEX_SHADER);
    if (vertexShaders[i] == 0) {
      succeeded = false;
    }
  }
  for (int i = 0; i < fs_count && succeeded; i++) {
    fragmentShaders[i] = loadShader(FragmentShaderNames[i],
                                  FragmentShaderSource[i],
                                  GL_FRAGMENT_SHADER);
    if (fragmentShaders[i] == 0) {
      succeeded = false;
    }
  }

  for (int i = 0; i < program_count && succeeded; i++) {
    shared_ptr<PathfinderShaderProgram> program = make_shared<PathfinderShaderProgram>();
    if(program->load(PROGRAM_INFO[i].name,
                     vertexShaders[PROGRAM_INFO[i].vertex],
                     fragmentShaders[PROGRAM_INFO[i].fragment])) {
      mPrograms.push_back(program);
    } else {
      succeeded = false;
    }
  }

  for(int i = 0; i < vs_count; i++) {
    GLDEBUG(glDeleteShader(vertexShaders[i]));
  }
  for(int i = 0; i < fs_count; i++) {
    GLDEBUG(glDeleteShader(fragmentShaders[i]));
  }
  return succeeded;
}

std::shared_ptr<PathfinderShaderProgram>
ShaderManager::getProgram(ProgramID aProgramID)
{
  assert(aProgramID >= 0);
  assert(aProgramID < program_count);

  return mPrograms[aProgramID];
}

PathfinderShaderProgram::PathfinderShaderProgram()
  : mProgram(0)
{

}

PathfinderShaderProgram::~PathfinderShaderProgram()
{
  if (mProgram) {
    glDeleteProgram(mProgram);
  }
}

bool
PathfinderShaderProgram::load(const char* aProgramName,
                              GLuint aVertexShader,
                              GLuint aFragmentShader)
{
  // TODO(kearwood) replace stderr output with logging function
  mProgramName = aProgramName;
  GLDEBUG(mProgram = glCreateProgram());
  GLDEBUG(glAttachShader(mProgram, aVertexShader));
  GLDEBUG(glAttachShader(mProgram, aFragmentShader));
  GLDEBUG(glLinkProgram(mProgram));

  // Report any link issues to stderr
  GLint link_success = GL_FALSE;
  GLDEBUG(glGetProgramiv(mProgram, GL_LINK_STATUS, &link_success));
  
  if (!link_success) {
    // Report any linking issues to stderr
    fprintf(stderr, "Failed to link shader program: %s\n", aProgramName);
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
    return false;
  }

  GLint uniformCount = 0;
  GLDEBUG(glGetProgramiv(mProgram, GL_ACTIVE_UNIFORMS, &uniformCount));
  GLint attributeCount = 0;
  GLDEBUG(glGetProgramiv(mProgram, GL_ACTIVE_ATTRIBUTES, &attributeCount));


  for (GLint uniformIndex = 0; uniformIndex < uniformCount; uniformIndex++) {
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
  return true;
}


/*

ShaderMap
compileShaders()
{
  ShaderMap shaders;

*/
/*

const shaders: Partial<ShaderMap<Partial<UnlinkedShaderProgram>>> = {};

        for (const shaderKey of SHADER_NAMES) {
            for (const typeName of ['vertex', 'fragment'] as Array<'vertex' | 'fragment'>) {
                const type = {
                    fragment: this.gl.FRAGMENT_SHADER,
                    vertex: this.gl.VERTEX_SHADER,
                }[typeName];

                const source = shaderSources[shaderKey][typeName];
                const shader = this.gl.createShader(type);
                if (shader == null)
                    throw new PathfinderError("Failed to create shader!");

                this.gl.shaderSource(shader, commonSource + "\n#line 1\n" + source);
                this.gl.compileShader(shader);
                if (!this.gl.getShaderParameter(shader, this.gl.COMPILE_STATUS)) {
                    const infoLog = this.gl.getShaderInfoLog(shader);
                    throw new PathfinderError(`Failed to compile ${typeName} shader ` +
                                              `"${shaderKey}":\n${infoLog}`);
                }

                if (shaders[shaderKey] == null)
                    shaders[shaderKey] = {};
                shaders[shaderKey]![typeName] = shader;
            }
        }

        return shaders as ShaderMap<UnlinkedShaderProgram>;

*/


/*

-----

const char* shader_blit_vs =
#include "resources/shaders/blit.vs.glsl"
;

const char* shader_blit_linear_fs =
#include "resources/shaders/blit-linear.fs.glsl"
;

void pathfinder_init()
{
  GLDEBUG(GLuint vs = glCreateShader(GL_VERTEX_SHADER));
  GLDEBUG(glShaderSource(vs, 1, &shader_blit_vs, NULL));
  GLDEBUG(glCompileShader(vs));
  GLDEBUG(GLuint fs = glCreateShader(GL_FRAGMENT_SHADER));
  GLDEBUG(glShaderSource(fs, 1, &shader_blit_linear_fs, NULL));
  GLDEBUG(glCompileShader(fs));
  GLDEBUG(GLuint shader_program = glCreateProgram());
  GLDEBUG(glAttachShader(shader_program, fs));
  GLDEBUG(glAttachShader(shader_program, vs));
  GLDEBUG(glLinkProgram(shader_program));
}


-----

*/
/*
  return shaders;
}

ShaderMap
linkShaders(pathfinder::ShaderMap &aShaders)
{
  ShaderMap shaderPrograms;
*/
  /*

          const shaderProgramMap: Partial<ShaderMap<PathfinderShaderProgram>> = {};
        for (const shaderName of Object.keys(shaders) as Array<keyof ShaderMap<string>>) {
            shaderProgramMap[shaderName] = new PathfinderShaderProgram(this.gl,
                                                                       shaderName,
                                                                       shaders[shaderName]);
        }
        return shaderProgramMap as ShaderMap<PathfinderShaderProgram>;

  */
/*
  return shaderPrograms;
}
*/


} // namepsace pathfinder
