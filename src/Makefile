CC  = gcc
CXX = g++

# Additional include directories
INCLUDES = include

# Compilation options:
CFLAGS   = # -Wall $(INCLUDES)
CXXFLAGS = -std=c++11 -Wall -O3 # $(INCLUDES)

# Linking options:
LDFLAGS = -L lib

# Libraries needed to link with
LDLIBS = -lSDL2 -lm


target: safe_queue.h
	$(CXX) $(CXXFLAGS) main.cpp -o main $(LDLIBS)


.PHONY: clean
clean:
	rm -f main


.PHONY: all
all: clean target
