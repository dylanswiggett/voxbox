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
#include "CombineVoxelData.h"
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
  signal(SIGABRT, handler);
  
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

  CombineVoxelData d;
  d.addVoxels(new BoxVoxelData(vec3(5,1,1), vec3(13,18,5), Voxel(vec3(.6,.2,0), 0, 255)));
  d.addVoxels(new BoxVoxelData(vec3(1,1,5), vec3(5,18,13), Voxel(vec3(.6,.2,0), 0, 255)));
  d.addVoxels(new BoxVoxelData(vec3(11,15,10), vec3(2,3,1), Voxel(vec3(.6,.2,.4), 0, 255)));
  d.addVoxels(new BoxVoxelData(vec3(1,0,1), vec3(18,5,18), Voxel(vec3(0,1,.3), 20, 255)));
  d.addVoxels(new BoxVoxelData(vec3(10,5,13), vec3(3,10,3), Voxel(vec3(1,1,1), 0, 255)));
  d.addVoxels(new BoxVoxelData(vec3(11,5,13), vec3(3,9,3), Voxel(vec3(1,1,1), 0, 255)));
  d.addVoxels(new BoxVoxelData(vec3(12,5,13), vec3(3,8,3), Voxel(vec3(1,1,1), 0, 255)));
  d.addVoxels(new BoxVoxelData(vec3(13,5,13), vec3(3,7,3), Voxel(vec3(1,1,1), 0, 255)));
  d.addVoxels(new BoxVoxelData(vec3(14,5,13), vec3(3,6,3), Voxel(vec3(1,1,1), 0, 255)));
  d.addVoxels(new BoxVoxelData(vec3(14,10,17), vec3(2,2,2), Voxel(vec3(0,.3,.5), 0, 255)));
  VoxelShader *vs = new VoxelShader(&d, 0, 0, 0, 20, 20, 20, 60, 60, 60);
  //VoxelShader *vs = new VoxelShader(&d, 0, 0, 0, 20, 20, 20, 100, 100, 100);
  //VoxelShader *vs = new VoxelShader(&d, 0, 0, 0, 20, 20, 20, 200, 200, 200);
  
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
