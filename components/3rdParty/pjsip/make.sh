#!/bin/bash -xe

mkdir -p "${LEGATO_BUILD}/3rdParty/pjsip"
cp -rf ${LEGATO_ROOT}/3rdParty/pjsip/* ${LEGATO_BUILD}/3rdParty/pjsip

unset LD

cd "${LEGATO_BUILD}/3rdParty/pjsip"

./configure --host=$(${CC} -dumpmachine) \
            --prefix=${LEGATO_BUILD}/3rdParty/pjsip \
            --disable-libwebrtc \
            --disable-sound \
            --disable-resample \
            --disable-oss \
            --disable-video \
            --disable-small-filter \
            --disable-large-filter \
            --disable-speex-aec \
            --disable-l16-codec \
            --disable-gsm-codec \
            --disable-g722-codec \
            --disable-g7221-codec \
            --disable-speex-codec \
            --disable-ilbc-codec \
            --disable-sdl \
            --disable-ffmpeg \
            --disable-v4l2 \
            --disable-openh264 \
            --disable-opencore-amr \
            --disable-silk \
            --disable-opus \
            --disable-libyuv \
            --enable-shared

make clean
make dep
make
make install
