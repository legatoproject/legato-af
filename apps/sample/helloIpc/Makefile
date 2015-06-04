
SERVER_DEPENDS := printServer.adef printServer/server.c printServer/Component.cdef

CLIENT_DEPENDS := printClient.adef printClient/client.c printClient/Component.cdef

TARGETS := localhost ar7 wp7 ar86 wp85

.PHONY: all $(TARGETS)

all: $(TARGETS)

$(TARGETS): %: printServer.% printClient.%

$(foreach target,$(TARGETS),printServer.$(target)): $(SERVER_DEPENDS)
	mkapp printServer.adef -v -t $(patsubst printServer.%,%,$@)

$(foreach target,$(TARGETS),printClient.$(target)): $(CLIENT_DEPENDS)
	mkapp printClient.adef -v -t $(patsubst printClient.%,%,$@)

.PHONY: clean
clean:
	rm -rf _build* *.localhost *.ar7 *.wp7 *.ar86 *.wp85

