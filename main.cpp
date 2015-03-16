#include <iostream>
#include <glm/glm.hpp>

#include "SDL2/SDL.h"
#include "GL/glew.h"

#include "VoxelData.h"
#include "BoxVoxelData.h"

#define SDL_FLAGS SDL_INIT_EVERYTHING
#define SDL_OGL_FLAGS SDL_WINDOW_OPENGL
#define DEFAULT_WIDTH 1400
#define DEFAULT_HEIGHT 800
#define OPENGL_MAJOR_VERSION 4
#define OPENGL_MINOR_VERSION 3

using namespace std;
using namespace glm;

SDL_Window *init_SDL(int w, int h) {
  int res;
  SDL_Window *window;
  res = SDL_Init(SDL_FLAGS);
  if (res == -1)
    return NULL;

  window = SDL_CreateWindow("GL Whatnot", 0, 0, w, h,
			    SDL_OGL_FLAGS);

  // SUCCESS
  return window;
}

void destroy_SDL() {
  SDL_Quit();
}

SDL_GLContext init_GL(SDL_Window *window) {
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, OPENGL_MAJOR_VERSION);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, OPENGL_MINOR_VERSION);

  SDL_GLContext context = SDL_GL_CreateContext(window);
  glewExperimental = GL_TRUE;
  glewInit();

  return context;
}

void destroy_GL(SDL_GLContext context) {
  SDL_GL_DeleteContext(context);
}

int main(int argc, char** argv)
{
  SDL_Window *window;
  SDL_GLContext glcontext;
  SDL_Event event;
  bool running;

  if (!(window = init_SDL(DEFAULT_WIDTH, DEFAULT_HEIGHT))) {
    cout << "Failed to initialize SDL. Exiting." << endl;
    return -1;
  }

  if (!(glcontext = init_GL(window))) {
    cout << "Failed to initialize openGL. Exiting." << endl;
    destroy_SDL();
    return -1;
  }

  // TODO: Initial setup here
  
  running = true;
  // Main event/draw loop.
  while (running) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
	running = false;
      }
    }

    // TODO: Run routine draw here.

    SDL_GL_SwapWindow(window);
    SDL_Delay(10.0f);
  }

  destroy_GL(glcontext);
  destroy_SDL();
}
