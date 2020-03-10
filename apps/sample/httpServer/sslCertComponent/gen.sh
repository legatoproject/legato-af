#!/bin/sh -xe

OUTPUT=$1

if [ ! -e "$OUTPUT" ]; then

    # Use config file from LEGATO toolchain path, when available.
    if [ -n "${TOOLCHAIN_ROOT}" ]; then
        export OPENSSL_CONF="$TOOLCHAIN_ROOT/etc/ssl/openssl.cnf"
        if [ ! -e "${OPENSSL_CONF}" ]; then
            echo "Error: config file, $TOOLCHAIN_ROOT/etc/ssl/openssl.cnf, not found" >&2
            exit 1
        fi
    fi

    # Generate certificate
    openssl req -new -x509 \
                -keyout "$OUTPUT" \
                -out "$OUTPUT" \
                -days 365 \
                -nodes \
                -subj '/CN=legato'
    chmod 400 "$OUTPUT"
fi
