# HTTP Server

## Notes

This app is configured to run on:
https://*:8443
http://*:8080

Mismatching protocol and port won't redirect you to a correct one, as PCRE is not compiled with these instructions.

Unfortunately Legato's minimal Python 2 installation doesn't include the cgi module, but as the included .py scripts show, it's not hard to do without.

## How to build

### Makefile

The Makefile contains a script to download and build the latest lighttpd version.

It also generates a default self-signed certificate for SSL: cfg/server.pem

### Manually

Download source from
# https://www.lighttpd.net/download/

Run these:

```
$ CCFLAGS="-DDEBUG -g" CC=$WP85_TOOLCHAIN_DIR/arm-poky-linux-gnueabi-gcc RANLIB=$WP85_TOOLCHAIN_DIR/arm-poky-linux-gnueabi-ranlib STRIP=$WP85_TOOLCHAIN_DIR/arm-poky-linux-gnueabi-strip ./configure --host=arm-poky-linux --enable-static  --enable-shared --without-zlib --without-bzip2 --without-pcre --with-openssl --with-openssl-libs=/usr/lib
$ make
```

Copy src/lighttpd to a binaries/ folder within the sample app directory (httpServer/binaries) and chmod +x
Copy src/.libs/*.so or whatever module libraries you need to httpServer/binaries/lib/
At the minimum, these are needed for the app to run:
mod_cgi.so
mod_dirlisting.so
mod_indexfile.so
mod_staticfile.so
mod_status.so

## Configuration

For SSL cert certificate generation, see https://redmine.lighttpd.net/projects/1/wiki/HowToSimpleSSL

The config is set to expect a cert called server.pem in the cfg directory.
