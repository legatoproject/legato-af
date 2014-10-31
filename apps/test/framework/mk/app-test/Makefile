.PHONY: all clean

TARGETS += localhost wp7 ar7

$(TARGETS): export TARGET = $@

all: $(TARGETS)

.PHONY: $(TARGETS)
$(TARGETS): %: libexternal/libexternal.%.so
	mkapp mkappTest.adef -v -w build/$@ -t $@ -o $(CURDIR) --cflags="'-g' -O0" --ldflags=-lrt

libexternal/libexternal.%.so:
	$(MAKE) -C libexternal $*

clean:
	rm -rf work staging obj bin lib build mix fromSource fromComponents *.ar7 *.localhost
	$(MAKE) -C libexternal clean

