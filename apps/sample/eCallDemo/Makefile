
TARGETS := ar7 ar86 wp85 localhost wp750x

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
	mkapp -v -t $@ \
		-i $(LEGATO_ROOT)/interfaces/positioning\
		-i $(LEGATO_ROOT)/interfaces/modemServices\
		-i eCallAppComponent \
		eCallDemo.adef

clean:
	rm -rf _build_* *.update