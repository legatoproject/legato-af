TARGETS := $(MAKECMDGOALS)

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
	mkapp -v -t $@ \
		atClientIntegrationTest.adef

clean:
	rm -rf _build_* *.update
