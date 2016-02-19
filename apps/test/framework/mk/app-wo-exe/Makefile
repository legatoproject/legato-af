.PHONY: all clean

TARGETS += localhost ar7 wp85

$(TARGETS): export TARGET = $@

all: $(TARGETS)

.PHONY: $(TARGETS)
$(TARGETS): %: libexternal/libexternal.%.so
	mkapp mkappTest.adef -v -t $@

libexternal/libexternal.%.so:
	$(MAKE) -C libexternal $*

clean:
	rm -rf _build* *.update
	$(MAKE) -C libexternal clean
