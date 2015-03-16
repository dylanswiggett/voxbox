CXX = g++
CFLAGS = -Wall -g -std=c++11
LINKERS = -lSDL2 -lGL -lGLU -lGLEW
OUT = voxbox

all: main
	$(CXX) $(CFLAGS) -o $(OUT) main.o $(LINKERS)

clean:
	rm -f *.o

main: main.cpp
	$(CXX) $(CFLAGS) -c -o main.o main.cpp
