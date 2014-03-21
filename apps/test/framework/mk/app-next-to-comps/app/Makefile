.PHONY: all ar7 clean

all: ../libexternal/libexternal.so
	mkapp mkappTest.adef -v -w build/localhost -i .. -c ..

ar7:
	mkapp -c .. mkappTest.adef -i .. -v -t ar7 -o $(CURDIR)

../libexternal/libexternal.so:
	$(MAKE) -C ../libexternal

clean:
	rm -rf work staging obj bin lib build mix fromSource fromComponents mkappTest.ar7 mkappTest.localhost
	$(MAKE) -C ../libexternal clean

