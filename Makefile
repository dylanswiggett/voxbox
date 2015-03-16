CXX = g++
CFLAGS = -Wall -g -std=c++11
LINKERS = -lSDL2 -lGL -lGLU -lGLEW
OUT = voxbox

ALL_FILES = main.o shader_loader.o

all: $(ALL_FILES)
	$(CXX) $(CFLAGS) -o $(OUT) $(foreach file, $(ALL_FILES), $(file)) $(LINKERS)

clean:
	rm -f *.o

main.o: main.cpp
	$(CXX) $(CFLAGS) -c -o main.o main.cpp

shader_loader.o: shader_loader.cpp shader_loader.h
	$(CXX) $(CFLAGS) -c -o shader_loader.o shader_loader.cpp
