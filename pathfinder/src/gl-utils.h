#ifndef PATHFINDER_GL_UTILS_H
#define PATHFINDER_GL_UTILS_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <map>

void setTextureParameters(GLint aFilter);
typedef std::map<std::string, GLint> UniformMap;

#endif // PATHFINDER_GL_UTILS_H
