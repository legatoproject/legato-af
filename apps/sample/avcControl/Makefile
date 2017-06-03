TARGETS := ar7 ar86 wp85 localhost wp750x ar758x ar759x virt

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
	mkapp -v -t $@ \
		avcControl.adef

clean:
	rm -rf _build_* *.update
