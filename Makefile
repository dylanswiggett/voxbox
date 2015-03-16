CXX = g++
CFLAGS = -Wall -g -std=c++11
LINKERS = -lSDL2 -lGL -lGLU -lGLEW
OUT = voxbox

ALL_FILES = main.o shader_loader.o

all: voxbox
	@echo "VoxBox compiled"

mktmp:
	@mkdir -p tmp

voxbox: mktmp $(foreach file, $(ALL_FILES), tmp/$(file))
	$(CXX) $(CFLAGS) -o $(OUT) $(foreach file, $(ALL_FILES), tmp/$(file)) $(LINKERS)

clean:
	rm -rf tmp

tmp/main.o: src/main.cpp
	$(CXX) $(CFLAGS) -c -o tmp/main.o src/main.cpp

tmp/shader_loader.o: src/shader_loader.cpp src/shader_loader.h
	$(CXX) $(CFLAGS) -c -o tmp/shader_loader.o src/shader_loader.cpp
