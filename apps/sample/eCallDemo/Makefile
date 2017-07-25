TARGETS := $(MAKECMDGOALS)

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
	mkapp -v -t $@ \
		-i $(LEGATO_ROOT)/interfaces/positioning \
		-i $(LEGATO_ROOT)/interfaces/modemServices \
		-i eCallAppComponent \
		eCallDemo.adef

clean:
	rm -rf _build_* *.update

