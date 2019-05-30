CC  = gcc
CXX = g++

# Additional include directories
# INCLUDES =

# Compilation options:
CFLAGS   = -Wall $(INCLUDES)
CXXFLAGS = -Wall $(INCLUDES)

# Linking options:
# -g for debugging info

LDFLAGS = -g

# List the libraries you need to link with in LDLIBS
# e.g., use "-lm" for the math library

# LDLIBS =

server: server.o


.PHONY: clean
clean:
	rm -f server server.o


.PHONY: all
all: clean server
