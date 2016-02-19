TARGETS := localhost ar7 wp85

.PHONY: all $(TARGETS) clean

VERBOSE := -v

SERVERS := $(foreach target,$(TARGETS),$(target)_server)
CLIENTS := $(foreach target,$(TARGETS),$(target)_client)

all: $(TARGETS)

$(TARGETS): %: %_client %_server

$(SERVERS): %_server:
	mkapp server.adef $(VERBOSE) -i interfaces -t $*

$(CLIENTS): %_client:
	mkapp client.adef $(VERBOSE) -i interfaces -t $*

clean:
	rm -rf _build_* *.update

