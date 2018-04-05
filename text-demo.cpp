// text-demo.cpp
//
// Copyright © 2017 The Pathfinder Project Developers.
//
// Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
// http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
// <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
// option. This file may not be copied, modified, or distributed
// except according to those terms.

#include "text-demo.h"
#include "eb_garamond_ttf.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>

#include <pathfinder.h>

using namespace pathfinder;
using namespace std;
using namespace kraken;

const char DEFAULT_TEXT[] =
R"(
  ’Twas brillig, and the slithy toves
  Did gyre and gimble in the wabe;
  All mimsy were the borogoves,
  And the mome raths outgrabe.

  “Beware the Jabberwock, my son!
  The jaws that bite, the claws that catch!
  Beware the Jubjub bird, and shun
  The frumious Bandersnatch!”

  He took his vorpal sword in hand:
  Long time the manxome foe he sought—
  So rested he by the Tumtum tree,
  And stood awhile in thought.

  And as in uffish thought he stood,
  The Jabberwock, with eyes of flame,
  Came whiffling through the tulgey wood,
  And burbled as it came!

  One, two! One, two! And through and through
  The vorpal blade went snicker-snack!
  He left it dead, and with its head
  He went galumphing back.

  “And hast thou slain the Jabberwock?
  Come to my arms, my beamish boy!
  O frabjous day! Callooh! Callay!”
  He chortled in his joy.

  ’Twas brillig, and the slithy toves
  Did gyre and gimble in the wabe;
  All mimsy were the borogoves,
  And the mome raths outgrabe.
)";

const float INITIAL_FONT_SIZE = 72.0f;

TextDemo::TextDemo()
  : mWindow(nullptr)
{

}

bool
TextDemo::init()
{
  if (!glfwInit()) {
    fprintf(stderr, "ERROR: could not start GLFW3\n");
    return false;
  }

#if defined(__APPLE__)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

  mWindow = glfwCreateWindow(1024, 768, "Pathfinder Test", NULL, NULL);
  if (!mWindow) {
    fprintf(stderr, "ERROR: could not open window with GLFW3\n");
    glfwTerminate();
    return false;
  }
  glfwMakeContextCurrent(mWindow);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    fprintf(stderr, "Failed to initialize OpenGL context\n");
    return false;
  }

  const GLubyte* renderer = glGetString(GL_RENDERER);
  const GLubyte* version = glGetString(GL_VERSION);
  printf("Renderer: %s\n", renderer);
  printf("OpenGL version supported %s\n", version);

  mFont = make_shared<Font>();
  if (!mFont->load(eb_garamond_ttf, eb_garamond_bin_len)) {
    return false;
  }

  mTextView.setFont(mFont);
  mTextView.setFontSize(INITIAL_FONT_SIZE);
  mTextView.setText(DEFAULT_TEXT);

  return true;
}

void
TextDemo::run()
{
  if (init()) {
    while(!glfwWindowShouldClose(mWindow)) {
      renderFrame();

      glfwPollEvents();
      glfwSwapBuffers(mWindow);
    }
  }

  shutdown();
}

void
TextDemo::shutdown()
{
  if(mWindow) {
    glfwDestroyWindow(mWindow);
    mWindow = nullptr;
  }

  // close GL context and any other GLFW resources
  glfwTerminate();
}

TextDemo::~TextDemo()
{
  shutdown();
}

void
TextDemo::renderFrame()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
}

int main(int argc, char **argv)
{
  TextDemo demo;
  demo.run();

  return 0;
}

