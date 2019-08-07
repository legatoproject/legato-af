TARGETS := $(MAKECMDGOALS)

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
    mkapp -v -t $@ \
        fileStreamDemo.adef

clean:
    rm -rf _build_* *.update
