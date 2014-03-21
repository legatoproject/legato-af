.PHONY: all ar7 clean

all: libexternal/libexternal.so
	mkapp mkappTest.adef -v -w build/localhost --cflags="'-g' -O0" --ldflags=-lrt

ar7:
	mkapp mkappTest-ar7.adef -v -w build/ar7 -t ar7 -o $(CURDIR)

libexternal/libexternal.so:
	$(MAKE) -C libexternal

clean:
	rm -rf work staging obj bin lib build mix fromSource fromComponents *.ar7 *.localhost
	$(MAKE) -C libexternal clean

