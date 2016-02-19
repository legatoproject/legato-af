
TARGETS := ar7 ar86 wp85 localhost

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
	mkapp -v -t $@ \
		-i $(LEGATO_ROOT)/interfaces/modemServices \
		modemDemo.adef

clean:
	rm -rf _build_* *.update