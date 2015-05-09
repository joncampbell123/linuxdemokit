
# what to build
TARGETS=vhello1 vhello2 vhello3 vhello4 vhello5

# how to compile/link to SDL2
SDL2_CFLAGS=`pkg-config sdl2 --cflags`
SDL2_LIBS=`pkg-config sdl2 --libs`

# libpng
PNG_CFLAGS=`pkg-config libpng --cflags`
PNG_LIBS=`pkg-config libpng --libs`

# libjpeg
JPEG_CFLAGS=
JPEG_LIBS=-ljpeg

LD ?= ld
CC ?= gcc
CXX ?= g++

LDFLAGS = $(SDL2_LIBS) $(PNG_LIBS) $(JPEG_LIBS)
CFLAGS = $(SDL2_CFLAGS) $(PNG_CFLAGS) $(JPEG_CFLAGS) -g3
CXXFLAGS = $(SDL2_CFLAGS) $(PNG_CFLAGS) $(JPEG_CFLAGS)

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
	$(CC) -o $@ $< $(LDFLAGS)

# vhello2
vhello2: vhello2.o
	$(CC) -o $@ $< $(LDFLAGS)

# vhello3
vhello3: vhello3.o
	$(CC) -o $@ $< $(LDFLAGS)

# vhello4
vhello4: vhello4.o
	$(CC) -o $@ $< $(LDFLAGS)

# vhello5
vhello5: vhello5.o
	$(CC) -o $@ $< $(LDFLAGS)

