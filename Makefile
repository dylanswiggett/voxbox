CXX = g++
CFLAGS = -g -Wall -std=c++11 -rdynamic
EXEC = main
SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)
LINKERS = -lsfml-window -lsfml-graphics -lsfml-system -lGL -lGLU -lGLEW

# Main target
$(EXEC): $(OBJECTS)
	$(CXX) $(CFLAGS) $(OBJECTS) -o $(EXEC) $(LINKERS)

%.o: %.cpp
	$(CXX) -c $(CFLAGS) $< -o $@ $(LINKERS)

# To remove generated files
clean:
	rm -f $(EXEC) $(OBJECTS)
