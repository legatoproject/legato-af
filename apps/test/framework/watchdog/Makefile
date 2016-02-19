# invoke with e.g.:
# make targ=ar7
# or whatever the target happens to be

test.$(targ): dogTest.$(targ) dogTestRevertAfterTimeout.$(targ) dogTestNeverNow.$(targ) dogTestNever.$(targ) dogTestWolfPack.$(targ)

%.$(targ): %.adef
	mkapp $< -t $(targ)

clean:
	rm -rf _build_*
	rm -f *.update
