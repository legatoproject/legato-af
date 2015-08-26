.PHONY: all clean

TARGETS += localhost wp7 ar7 wp85

$(TARGETS): export TARGET = $@

all: $(TARGETS)

.PHONY: $(TARGETS)
$(TARGETS): %: libexternal/libexternal.%.so
	mkapp mkappTest.adef -v -t $@

libexternal/libexternal.%.so:
	$(MAKE) -C libexternal $*

clean:
	rm -rf _build* $(foreach target,$(TARGETS),*.$(target))
	$(MAKE) -C libexternal clean
