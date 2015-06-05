.PHONY: all ar7 clean

all:
	mkapp sandboxed.adef -v
	mkapp unsandboxed.adef -v

ar7:
	mkapp sandboxed.adef -v -t ar7
	mkapp unsandboxed.adef -v -t ar7

clean:
	rm -rf _build_* build_* *.ar7 *.localhost

