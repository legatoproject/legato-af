TARGETS := $(MAKECMDGOALS)

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
	mkapp -v -t $@ mqttPublisher.adef
	mkapp -v -t $@ mqttSubscriber.adef

clean:
	rm -rf _build_* *.*.update
