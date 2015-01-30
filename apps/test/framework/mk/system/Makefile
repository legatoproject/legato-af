.PHONY: all clean

TARGETS := localhost wp7 ar7

all: $(TARGETS)

$(TARGETS):
	@echo =========================
	@echo Building: $@
	@echo =========================
	mksys basic.sdef -t $@ -v -s app2 -i stuff -s stuff

clean:
	rm -rf _build_basic basic.*_sys
