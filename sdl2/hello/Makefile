
# what to build
TARGETS=vhello1

# how to compile/link to SDL2
SDL2_CFLAGS=`pkg-config sdl2 --cflags`
SDL2_LIBS=`pkg-config sdl2 --libs`

LD ?= ld
CC ?= gcc
CXX ?= g++

LDFLAGS = $(SDL2_LIBS)
CFLAGS = $(SDL2_CFLAGS)
CXXFLAGS = $(SDL2_CFLAGS)

# tell make what to do with .c and .cpp files
.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<
.cpp.o:
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

# default target.
all: $(TARGETS)

# "make clean" by convention. The '@' prevents make from echoing the command
clean:
	rm -fv *.o *.a *.la *.so
	rm -fv $(TARGETS)

# "make distclean" by convention. The '@' prevents make from echoing the command
distclean: clean

# vhello1
vhello1: vhello1.o
	$(CXX) -o $@ $< $(LDFLAGS)

