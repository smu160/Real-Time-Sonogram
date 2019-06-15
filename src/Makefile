CC  = gcc
CXX = g++

# Additional include directories
INCLUDES = include

# Compilation options:
CFLAGS   = -std=c++17 -Wall $(INCLUDES)
CXXFLAGS = -Wall $(INCLUDES)

# Linking options:
LDFLAGS = -L lib

# Libraries needed to link with
LDLIBS = -lSDL2 -lm


target:
	$(CXX) main.cpp -o main $(LDLIBS)


.PHONY: clean
clean:
	rm -f main


.PHONY: all
all: clean target
