
TARGETS := ar7 ar86 wp85 localhost wp750x

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
	mkapp -v -t $@ \
		fileInfo.adef

clean:
	rm -rf _build_* *.update