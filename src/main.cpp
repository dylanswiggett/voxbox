#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <ctime>

#include <glm/glm.hpp>
#include "GL/glew.h"
#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>

#include "VoxelShader.hpp"
#include "CombineVoxelData.h"
#include "BoxVoxelData.h"
#include "SinVoxelData.hpp"
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
  std::cout << "HERE?" << std::endl;
  
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

  /*
  d.addVoxels(new BoxVoxelData(ivec3(6,0,6), ivec3(9, 3, 9), Voxel(vec3(.6, .6, 1), 0, 255)));
  d.addVoxels(new BoxVoxelData(ivec3(4,0,4), ivec3(13, 2, 13), Voxel(vec3(.6, .6, 1), 0, 255)));
  d.addVoxels(new BoxVoxelData(ivec3(9,3,9), ivec3(3, 3, 3), Voxel(vec3(1, 1, 1), 100, 0)));
  d.addVoxels(new BoxVoxelData(ivec3(6,3,6), ivec3(3, 3, 3), Voxel(vec3(.6, .6, .6), 0, 0)));
  d.addVoxels(new BoxVoxelData(ivec3(6,3,12), ivec3(3, 3, 3), Voxel(vec3(.6, .6, .6), 0, 0)));
  d.addVoxels(new BoxVoxelData(ivec3(12,3,6), ivec3(3, 3, 3), Voxel(vec3(.6, .6, .6), 0, 0)));
  d.addVoxels(new BoxVoxelData(ivec3(12,3,12), ivec3(3, 3, 3), Voxel(vec3(.6, .6, .6), 0, 0)));
  d.addVoxels(new BoxVoxelData(ivec3(8,6,8), ivec3(5, 1, 5), Voxel(vec3(.3, .3, .3), 0, 255)));
  d.addVoxels(new BoxVoxelData(ivec3(9,6,9), ivec3(3, 2, 3), Voxel(vec3(.3, .3, .3), 0, 255)));
  d.addVoxels(new BoxVoxelData(ivec3(10,6,10), ivec3(1, 3, 1), Voxel(vec3(.3, .3, .3), 0, 255)));
  d.addVoxels(new BoxVoxelData(ivec3(-5,0,-5), ivec3(30, 1, 30), Voxel(vec3(1,.6,.6), 0, 255)));
  d.addVoxels(new BoxVoxelData(ivec3(-15,0,-15), ivec3(80, 1, 50), Voxel(vec3(.6,1,.6), 0, 255)));

  for (int i = 0; i < 10; i++) {
    d.addVoxels(new BoxVoxelData(ivec3(30 + i * .5 , 1.5 * i + 1,i * .5),
				 ivec3(10 - i, 2, 10 - i),
				 Voxel(vec3(.6,1.0 - .1 * i,.6), 0, 255)));
    d.addVoxels(new BoxVoxelData(ivec3(25 + i * .5 , i + 1,13 + i * .5),
				 ivec3(10 - i, 2, 10 - i),
				 Voxel(vec3(.6,1.0 - .1 * i,.6), 0, 255)));
  }
  */

  d.addVoxels(new SinVoxelData(10, 5, 10, Voxel(vec3(.6,1,.6), 0, 100)));
  //d.addVoxels(new SinVoxelData(2, 5, 13, Voxel(vec3(.3,.3,.7), 0, 255)));
  d.addVoxels(new BoxVoxelData(ivec3(-10000,0,-10000), ivec3(20000,1,20000), Voxel(vec3(.3,.3,.7), 0, 255)));

  d.addVoxels(new BoxVoxelData(ivec3(10,30,10), ivec3(10,1,10), Voxel(vec3(.6,.6,.6), 0, 255)));
  d.addVoxels(new BoxVoxelData(ivec3(10,49,10), ivec3(10,1,10), Voxel(vec3(.6,.6,.6), 0, 255)));
  d.addVoxels(new BoxVoxelData(ivec3(0,30,40), ivec3(10,1,10), Voxel(vec3(.6,.6,.6), 0, 255)));
  d.addVoxels(new BoxVoxelData(ivec3(0,49,40), ivec3(10,1,10), Voxel(vec3(.6,.6,.6), 0, 255)));
  
  //d.addVoxels(new BoxVoxelData(ivec3(4,0,44), ivec3(2,30,2), Voxel(vec3(.6,.6,.6), 0, 255)));
  d.addVoxels(new BoxVoxelData(ivec3(10,30,10), ivec3(10,20,10), Voxel(vec3(1,.6,.6), 100, 0)));
  d.addVoxels(new BoxVoxelData(ivec3(0,30,40), ivec3(10,20,10), Voxel(vec3(.6,.6,1), 100, 0)));
  d.addVoxels(new BoxVoxelData(ivec3(14,0,14), ivec3(2,30,2), Voxel(vec3(.6,.6,.6), 0, 255)));
  d.addVoxels(new BoxVoxelData(ivec3(4,0,44), ivec3(2,30,2), Voxel(vec3(.6,.6,.6), 0, 255)));

  VoxelShader *vs = new VoxelShader(&d, 0, 0, 0, 60, 60, 60);
  //VoxelShader *vs = new VoxelShader(&d, 0, 0, 0, 20, 20, 20, 100, 100, 100);
  //VoxelShader *vs = new VoxelShader(&d, 0, 0, 0, 20, 20, 20, 200, 200, 200);
  
  running = true;
  float xoff = 0;//10000;
  float yoff = 0;//10000;
  float xsp = 0;
  float ysp = 0;
  float speed = 1;
  bool update = true;
  // Main event/draw loop.
  while (running) {
    while (window.pollEvent(event)) {
      switch (event.type) {
      case sf::Event::Closed:
	running = false;
	break;
      case sf::Event::KeyPressed:
	if (event.key.code == sf::Keyboard::Q) running = false;
	if (event.key.code == sf::Keyboard::Space) update = !update;
	if (event.key.code == sf::Keyboard::Right) xsp = speed;
	if (event.key.code == sf::Keyboard::Left) xsp = -speed;
	if (event.key.code == sf::Keyboard::Up) ysp = speed;
	if (event.key.code == sf::Keyboard::Down) ysp = -speed;
	break;
      case sf::Event::KeyReleased:
	if (event.key.code == sf::Keyboard::Right) xsp = 0;
	if (event.key.code == sf::Keyboard::Left) xsp = 0;
	if (event.key.code == sf::Keyboard::Up) ysp = 0;
	if (event.key.code == sf::Keyboard::Down) ysp = 0;
	break;
      default:
	break;
      }
    }

    xoff -= ysp - xsp;
    yoff -= ysp + xsp;
    /*
    xoff += xsp;
    yoff -= ysp;
    */

    // TODO: Run routine draw here.
    sf::Vector2u wsize = window.getSize();
    vs->draw(wsize.x, wsize.y, xoff, yoff, update);

    window.display();
  }

  delete vs;
}
