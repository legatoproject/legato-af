.PHONY: all ar7 clean

all:
	mkapp ipc.adef -v -i interfaces/hello

ar7:
	mkapp ipc.adef -v -t ar7 -i interfaces/hello

clean:
	rm -rf work staging obj bin lib _build_* *.update build_ipc

