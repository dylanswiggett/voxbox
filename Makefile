CXX = g++
CFLAGS = -Wall -g -std=c++11
LINKERS = -lSDL2 -lGL -lGLU
OUT = voxbox

all: main
	$(CXX) $(CFLAGS) $(LINKERS) -o $(OUT) main.o

clean:
	rm -f *.o

main: main.cpp
	$(CXX) -c $(CFLAGS) -o main.o main.cpp
