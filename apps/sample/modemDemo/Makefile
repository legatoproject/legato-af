TARGETS := ar7 wp7 ar86 wp85 localhost

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
	export TARGET=$@ ; \
	mkapp -v -t $@ \
		  -i $(LEGATO_ROOT)/interfaces/modemServices/ \
		  -i $(LEGATO_ROOT)/interfaces/dataConnectionService/ \
		  modemDemo.adef

clean:
	rm -rf _build_* *.ar7 *.wp7 *.ar86 *.wp85 *.localhost
