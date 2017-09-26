TARGETS = \
	ar7 \
	ar758x \
	ar759x \
	ar86 \
	wp750x \
	wp76xx \
	wp85 \
	raspi \
	virt \
	localhost

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
	mkapp -v -t $@ \
		-i $(LEGATO_ROOT)/interfaces/modemServices \
		httpGet.adef

clean:
	rm -rf _build_* *.update
