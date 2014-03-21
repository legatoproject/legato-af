.PHONY: all clean test1 ar7

all: test1

test1:  lib/libexternal.so
	mkdir -p obj
	mkdir -p lib
	mkdir -p bin
	mkexe -v -o bin/foo foo.c lib/libexternal.so -w obj -l lib
	mkexe -v -o bin/munchkin munchkin -w obj -l lib
	mkexe -v -o bin/fandm foo.c munchkin lib/libexternal.so -w obj -l lib

lib/libexternal.so:
	mkdir -p lib
	gcc -fPIC -shared -o $@ libexternal.c


clean:
	rm -rf obj lib bin ar7


ar7:
	mkdir -p ar7/obj
	mkdir -p ar7/lib
	mkdir -p ar7/bin
	mkexe -v -o ar7/bin/munchkin munchkin -w ar7/obj -l ar7/lib -t ar7

