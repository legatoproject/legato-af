TARGETS := $(MAKECMDGOALS)

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
	mkapp -v -t $@ \
		streamMedia.adef
	mkapp -v -t $@ \
		streamMediaTest.adef

clean:
	rm -rf _build_* *.update
