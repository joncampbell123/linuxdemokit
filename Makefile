
# SUBDIRS
SUBDIRS=sdl2

# default target.
# as a bunch of separate projects with wildly varying dependencies it's probably not a good idea to compile it all (yet) until we get autoconf built up
all:
	echo Please cd into a directory and build something

# "make clean" by convention. The '@' prevents make from echoing the command
clean:
	@for i in $(SUBDIRS); do make -C $$i clean || break; done

# "make distclean" by convention. The '@' prevents make from echoing the command
distclean: clean
	@for i in $(SUBDIRS); do make -C $$i distclean || break; done
	find -name \*~ -delete

