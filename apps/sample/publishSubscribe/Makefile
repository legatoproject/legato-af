.PHONY: all
all:
	cd ps; \
	mksys -t $(TARGET_TYPE) ps.sdef;
	cd ..;
	cp ./ps/ps.$(TARGET_TYPE).update .;

clean:
	cd ps; \
	rm -rf _build_* *.update
	cd ..;
	rm -rf _build_* *.update
