.PHONY: all fail succeed clean

all: fail succeed

succeed:
	mkapp hello.adef -v -g hi:1234
	mkapp hello.adef -v -g hey:4321 --debug=allo:44444

fail:
	if mkapp hello.adef -v -g hi:123456 ; then echo "** Should have failed" ; false ; else true ; fi
	if mkapp hello.adef -v -g hi:-1 ; then echo "** Should have failed" ; false ; else true ; fi
	if mkapp hello.adef -v -g hi:0 ; then echo "** Should have failed" ; false ; else true ; fi
	if mkapp hello.adef -v -g hey:44444 --debug=allo:44444 ; then echo "** Should have failed" ; false ; else true ; fi

clean:
	rm -rf build_* *.ar7 *.localhost

