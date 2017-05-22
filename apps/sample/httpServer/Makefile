TARGETS = \
	ar7 \
	ar758x \
	ar759x \
	ar86 \
	wp750x \
	wp76xx \
	wp85 \
	raspi \
	virt \
	localhost

ifneq ($(MAKECMDGOALS),clean)
  export TOOLCHAIN_DIR := $(shell findtoolchain $(MAKECMDGOALS))
endif

_build_lighttpd.tar.gz:
	wget -O $@ http://download.lighttpd.net/lighttpd/releases-1.4.x/lighttpd-1.4.39.tar.gz

binaries/lighttpd: _build_lighttpd.tar.gz
	rm -rf _build_lighttpd
	mkdir _build_lighttpd
	tar zxvf _build_lighttpd.tar.gz -C _build_lighttpd --strip=1
	(cd _build_lighttpd; \
	 export CCFLAGS="-DDEBUG -g"; \
	 export CC=$(TOOLCHAIN_DIR)/arm-poky-linux-gnueabi-gcc; \
	 export RANLIB=$(TOOLCHAIN_DIR)/arm-poky-linux-gnueabi-ranlib; \
	 export STRIP=$(TOOLCHAIN_DIR)/arm-poky-linux-gnueabi-strip; \
		./configure --host=arm-poky-linux --target=arm-poky-linux --enable-static  --enable-shared --without-zlib --without-bzip2 --without-pcre --with-openssl --with-openssl-libs=/usr/lib && \
		make)
	rm -rf binaries
	mkdir -p binaries/lib
	cp _build_lighttpd/src/.libs/mod_{cgi,dirlisting,indexfile,staticfile,status}.so binaries/lib/
	cp _build_lighttpd/src/lighttpd binaries

cfg/server.pem:
	openssl req -new -x509 -keyout $@ -out $@ -days 365 -nodes -subj "/CN=legato"
	chmod 400 $@

$(TARGETS): binaries/lighttpd cfg/server.pem
	mkapp -v -t $@ httpServer.adef

clean:
	rm -rf _build_* *.update binaries
