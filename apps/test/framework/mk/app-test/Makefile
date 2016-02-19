.PHONY: all clean

TARGETS += localhost ar7 wp85

$(TARGETS): export TARGET = $@

all: $(TARGETS)

.PHONY: $(TARGETS)
$(TARGETS): %: libexternal/libexternal.%.so
	mkapp mkappTest.adef -v -w build/$@ -t $@ -o $(CURDIR) --cflags="'-g' -O0" --ldflags=-lrt

libexternal/libexternal.%.so:
	$(MAKE) -C libexternal $*

clean:
	rm -rf build *.update
	$(MAKE) -C libexternal clean

