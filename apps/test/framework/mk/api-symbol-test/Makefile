localhost:
	mkapp -v -t $@ app.adef

clean:
	rm -rf _build_app app.localhost

run: localhost
	stoplegato ; startlegato && \
	( config import users/$(USER)/bindings bindings.cfg && \
	  sdir load && sdir list && \
	  (_build_app/localhost/staging/bin/server &) && \
	  (_build_app/localhost/staging/bin/client &) && sleep 1 \
	) ; \
	stoplegato
