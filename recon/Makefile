CC  = gcc
CXX = g++

# Additional include directories
INCLUDES = include

# Compilation options:
CFLAGS   = -Wall $(INCLUDES)
CXXFLAGS = -Wall $(INCLUDES)

# Linking options:
# -g for debugging info

LDFLAGS = -L lib

# List the libraries you need to link with in LDLIBS
# e.g., use "-lm" for the math library

LDLIBS = -lSDL2 -lm


target:
	$(CXX) main.cpp -o main $(LDLIBS)


.PHONY: clean
clean:
	rm -f main


.PHONY: all
all: clean target
