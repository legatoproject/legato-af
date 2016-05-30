include ../common.mk

$(TARGET):
	mkapp -v -t $@ app.adef -w $(BUILD_DIR) -o $(BUILD_DIR)

run: localhost
	stoplegato ; startlegato && \
	( config import users/$(USER)/bindings bindings.cfg && \
	  sdir load && sdir list && \
	  (_build_app/localhost/staging/bin/server &) && \
	  (_build_app/localhost/staging/bin/client &) && sleep 1 \
	) ; \
	stoplegato
