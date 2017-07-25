TARGETS := $(MAKECMDGOALS)

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
	mkapp -v -t $@ \
		fwupdateDownloader.adef

clean:
	rm -rf _build_* *.update

