
# what to build
TARGETS=vhello1 vhello2

# how to compile/link to XCB
XCB_CFLAGS=`pkg-config xcb --cflags` `pkg-config xcb-shm --cflags` 
XCB_LIBS=`pkg-config xcb --libs` `pkg-config xcb-shm --libs` 

# libpng
PNG_CFLAGS=`pkg-config libpng --cflags`
PNG_LIBS=`pkg-config libpng --libs`

# libjpeg
JPEG_CFLAGS=
JPEG_LIBS=-ljpeg

LD ?= ld
CC ?= gcc
CXX ?= g++

LDFLAGS = $(XCB_LIBS) $(PNG_LIBS) $(JPEG_LIBS)
CFLAGS = $(XCB_CFLAGS) $(PNG_CFLAGS) $(JPEG_CFLAGS)
CXXFLAGS = $(XCB_CFLAGS) $(PNG_CFLAGS) $(JPEG_CFLAGS)

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

