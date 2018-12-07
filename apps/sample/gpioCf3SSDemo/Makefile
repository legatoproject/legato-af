TARGETS := $(MAKECMDGOALS)

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
	mkapp -v -t $@ \
            gpioCf3SSDemo.adef

clean:
	rm -rf _build_* *.update
