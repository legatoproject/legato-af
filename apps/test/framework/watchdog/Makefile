test.localhost: dogTest.localhost dogTestRevertAfterTimeout.localhost dogTestNeverNow.localhost dogTestNever.localhost dogTestWolfPack.localhost

dogTest.localhost: dogTest.adef dogTest/dogTest.c dogTest/Component.cdef
	mkapp dogTest.adef

dogTestRevertAfterTimeout.localhost: dogTestRevertAfterTimeout.adef  dogTestRevertAfterTimeout/dogTestRevertAfterTimeout.c dogTestRevertAfterTimeout/Component.cdef
	mkapp dogTestRevertAfterTimeout.adef

dogTestNever.localhost: dogTestNever.adef dogTestNever/dogTestNever.c dogTestNever/Component.cdef
	mkapp dogTestNever.adef

dogTestNeverNow.localhost: dogTestNeverNow.adef dogTestNeverNow/dogTestNeverNow.c dogTestNeverNow/Component.cdef
	mkapp dogTestNeverNow.adef

dogTestWolfPack.localhost: dogTestWolfPack.adef loneWolf/loneWolf.c loneWolf/Component.cdef
	mkapp dogTestWolfPack.adef
	mkapp dogTest.adef

test.ar7: dogTest.ar7 dogTestRevertAfterTimeout.ar7 dogTestNeverNow.ar7 dogTestNever.ar7 dogTestWolfPack.ar7

dogTest.ar7: dogTest.adef dogTest/dogTest.c dogTest/Component.cdef
	mkapp dogTest.adef -t ar7

dogTestRevertAfterTimeout.ar7: dogTestRevertAfterTimeout.adef dogTestRevertAfterTimeout/dogTestRevertAfterTimeout.c dogTestRevertAfterTimeout/Component.cdef
	mkapp dogTestRevertAfterTimeout.adef -t ar7

dogTestNever.ar7: dogTestNever.adef dogTestNever/dogTestNever.c dogTestNever/Component.cdef
	mkapp dogTestNever.adef -t ar7

dogTestNeverNow.ar7: dogTestNeverNow.adef dogTestNeverNow/dogTestNeverNow.c dogTestNeverNow/Component.cdef
	mkapp dogTestNeverNow.adef -t ar7

dogTestWolfPack.ar7: dogTestWolfPack.adef loneWolf/loneWolf.c loneWolf/Component.cdef
	mkapp dogTestWolfPack.adef -t ar7

clean:
	rm -rf _build_*
	rm -f *.ar7
	rm -f *.localhost
