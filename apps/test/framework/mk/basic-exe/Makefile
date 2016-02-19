.PHONY: all clean test1

all: test1

test1:  lib/libexternal.so
	mkdir -p build/localhost
	mkexe -v -o foo foo.c munchkin lib/libexternal.so
	mkexe -v -o foo2 foo.c munchkin lib/libexternal.so -w build --cflags="-g -O0"
	mkexe -v -o 'build/$$LEGATO_TARGET/foo' foo.c munchkin lib/libexternal.so

lib/libexternal.so:
	mkdir -p lib
	gcc -fPIC -shared -o $@ libexternal.c


clean:
	rm -rf build _build lib *.so foo foo2

