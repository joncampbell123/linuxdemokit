
# SUBDIRS
SUBDIRS=hello

# default target.
all:
	@for i in $(SUBDIRS); do make -C $$i all || break; done

# "make clean" by convention. The '@' prevents make from echoing the command
clean:
	@for i in $(SUBDIRS); do make -C $$i clean || break; done

# "make distclean" by convention. The '@' prevents make from echoing the command
distclean: clean
	@for i in $(SUBDIRS); do make -C $$i distclean || break; done

