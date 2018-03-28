TARGETS := $(MAKECMDGOALS)

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
	mkapp -v -t $@ -s $(LEGATO_ROOT)/components javaHelloWorld.adef

clean:
	rm -rf _build_* *.update

