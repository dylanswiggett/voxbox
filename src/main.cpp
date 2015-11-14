#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

#include <glm/glm.hpp>
#include "GL/glew.h"
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>

#include "VoxelShader.hpp"
#include "BoxVoxelData.h"
#include "VoxelData.h"

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 800
#define OPENGL_MAJOR_VERSION 4
#define OPENGL_MINOR_VERSION 3

using namespace std;
using namespace glm;


void handler(int sig) {
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

int main(int argc, char** argv)
{
  signal(SIGSEGV, handler);
  
  bool running;
  sf::Event event;

  sf::ContextSettings settings;
  //settings.depthBits = 24;
  //settings.stencilBits = 8;
  //settings.antialiasingLevel = 4;
  settings.majorVersion = OPENGL_MAJOR_VERSION;
  settings.minorVersion = OPENGL_MINOR_VERSION;

  sf::Window window(sf::VideoMode(DEFAULT_WIDTH, DEFAULT_HEIGHT), "VoxBox",
		    sf::Style::Default, settings);

  // TODO: Initial setup here
  window.setFramerateLimit(60);
  window.setVerticalSyncEnabled(true);
  window.setPosition(sf::Vector2i((sf::VideoMode::getDesktopMode().width/2)-DEFAULT_WIDTH/2,
				  (sf::VideoMode::getDesktopMode().height/2)-DEFAULT_HEIGHT/2));
  glewInit();

  BoxVoxelData d(vec3(10,10,0), vec3(5,10,5), vec3());
  VoxelShader *vs = new VoxelShader(&d, 0, 0, 0, 20, 20, 20, 20, 20, 20);
  
  running = true;
  // Main event/draw loop.
  while (running) {
    while (window.pollEvent(event)) {
      switch (event.type) {
      case sf::Event::Closed:
	running = false;
	break;
      default:
	break;
      }
    }

    // TODO: Run routine draw here.
    sf::Vector2u wsize = window.getSize();
    vs->draw(wsize.x, wsize.y);

    window.display();
  }

  delete vs;
}
