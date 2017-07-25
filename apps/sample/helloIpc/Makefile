TARGETS := $(MAKECMDGOALS)

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
	mkapp -v -t $@ printClient.adef
	mkapp -v -t $@ printServer.adef

clean:
	rm -rf _build_* *.update

