# HTTP Server

## Notes

This app is configured to run on:
https://*:8443
http://*:8080

Mismatching protocol and port won't redirect you to a correct one, as PCRE is not compiled with these instructions.

Unfortunately Legato's minimal Python 2 installation doesn't include the cgi module, but as the included .py scripts show, it's not hard to do without.

## How to build

### Makefile

The .adef uses a 'lighttpd' 3rd party component, that uses sources stored in 3rdParty/lighttpd/.

It also generates a default self-signed certificate for SSL: cfg/server.pem
