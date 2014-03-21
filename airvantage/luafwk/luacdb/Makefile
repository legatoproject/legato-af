CDBDIR=		../../../libs/c/cdb
LUADIR=		../liblua
CFLAGS=		-fpic -O2 -g -Wall -I$(CDBDIR) -I$(LUADIR) #-m32
CDBLIBS=	$(CDBDIR)/libcdb.a $(CDBDIR)/liballoc.a $(CDBDIR)/libbuffer.a $(CDBDIR)/libunix.a $(CDBDIR)/libbyte.a
LDFLAGS=	-s #-m32

.PHONY: all clean test

all:	cdb.so cdb_make.so

cdb.so:	lcdb.o
	$(CC) $(LDFLAGS) -o $@ -shared lcdb.o $(CDBLIBS) -lc

cdb_make.so:	lcdb_make.o
	$(CC) $(LDFLAGS) -o $@ -shared lcdb_make.o $(CDBLIBS) -lc

test:	cdb.so cdb_make.so
	./rts | sed -e 's/lcdb_values_aux: .*$$/lcdb_values_aux:/' >rts.out && cmp rts.out rts.exp

clean:
	-rm -f *.o *.so services.cdb 12.cdb rts.out
