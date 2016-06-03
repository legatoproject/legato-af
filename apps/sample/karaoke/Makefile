
TARGETS := ar7 ar86 wp85 localhost

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
	mkapp -v -t $@ \
		karaoke.adef

clean:
	rm -rf _build_* *.update
