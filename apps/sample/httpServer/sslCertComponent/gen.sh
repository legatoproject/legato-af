#!/bin/sh -xe

OUTPUT=$1

if [ ! -e "$OUTPUT" ]; then
    openssl req -new -x509 \
                -keyout "$OUTPUT" \
                -out "$OUTPUT" \
                -days 365 \
                -nodes \
                -subj '/CN=legato'
    chmod 400 "$OUTPUT"
fi
