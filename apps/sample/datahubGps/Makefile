TARGETS := wp85 wp750x wp76xx wp77xx

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
	mkapp -v -t $@ gpsSensor.adef
	mkapp -v -t $@ gpsMonitor.adef

clean:
	rm -rf _build_* *.*.update
