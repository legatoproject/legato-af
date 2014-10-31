TARGETS := localhost ar7 wp7

.PHONY: all $(TARGETS) clean

VERBOSE := -v

all: $(TARGETS)

$(TARGETS): %: %_test

%_test:
	mkapp test.adef $(VERBOSE) -t $*

clean:
	rm -rf _build_* $(foreach target,$(TARGETS),*.$(target))

