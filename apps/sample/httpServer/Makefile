TARGETS := $(MAKECMDGOALS)

$(TARGETS):
	mkapp -v -t $@ httpServer.adef

clean:
	rm -rf _build_* *.update binaries
