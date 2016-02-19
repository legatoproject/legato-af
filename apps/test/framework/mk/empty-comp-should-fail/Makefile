.PHONY: all app-test exe-test clean

all:
	$(MAKE) -k app-test || $(MAKE) -k exe-test

app-test:
	mkapp empty.adef
	mkapp void.adef

exe-test:
	mkdir -p bin
	mkexe -o bin/notMuch-exe void notMuch.c
	mkexe -o bin/void-exe void

clean:
	rm -rf build_* *.update _build* bin
