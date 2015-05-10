
# what to build
TARGETS=vhello1

# how to compile/link to X11
X11_CFLAGS=`pkg-config x11 --cflags` `pkg-config xext --cflags` 
X11_LIBS=`pkg-config x11 --libs` `pkg-config xext --libs` 

# libpng
PNG_CFLAGS=`pkg-config libpng --cflags`
PNG_LIBS=`pkg-config libpng --libs`

# libjpeg
JPEG_CFLAGS=
JPEG_LIBS=-ljpeg

LD ?= ld
CC ?= gcc
CXX ?= g++

LDFLAGS = $(X11_LIBS) $(PNG_LIBS) $(JPEG_LIBS)
CFLAGS = $(X11_CFLAGS) $(PNG_CFLAGS) $(JPEG_CFLAGS)
CXXFLAGS = $(X11_CFLAGS) $(PNG_CFLAGS) $(JPEG_CFLAGS)

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

