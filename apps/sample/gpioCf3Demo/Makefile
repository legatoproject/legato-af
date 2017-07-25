TARGETS := $(MAKECMDGOALS)

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
	mkapp -v -t $@ \
		gpioCf3Demo.adef

clean:
	rm -rf _build_* *.update

