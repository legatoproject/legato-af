#!/bin/bash

set -xe

if [ -z "$LEGATO_ROOT" ]; then
    echo "LEGATO_ROOT not set"
    exit 1
fi

SCRIPT_PATH="$( cd `dirname "$0"` && pwd )/`basename $0`"
if [ -L "$SCRIPT_PATH" ]; then
    SCRIPT_PATH=$(readlink $SCRIPT_PATH)
fi

VIRT_TARGET_ARCH=${VIRT_TARGET_ARCH:-x86}

## Create build directory

if [ -z "$BUILD_DIR" ]; then
    BUILD_DIR="$LEGATO_ROOT/build/virt-${VIRT_TARGET_ARCH}/docker"
fi

mkdir -p $BUILD_DIR

## Prepare files to be packaged in Docker image

# Configuration Script
cp "$LEGATO_ROOT/framework/tools/scripts/legato-qemu" "$BUILD_DIR"

# Download Legato image & Linux image
if [ -z "$SKIP_DOWNLOAD" ]; then
    export USER_DIR="$BUILD_DIR"

    ONLY_PREPARE=1 $BUILD_DIR/legato-qemu

    export LEGATO_IMG="$USER_DIR/img-virt-${VIRT_TARGET_ARCH}/legato.squashfs"
    export KERNEL_IMG="$USER_DIR/img-virt-${VIRT_TARGET_ARCH}/kernel"
    export ROOTFS_IMG="$USER_DIR/img-virt-${VIRT_TARGET_ARCH}/rootfs.qcow2"
fi

if [ ! -f "$LEGATO_IMG" ]; then
    echo "legato image ($LEGATO_IMG) not available, please build Legato before packaging the Docker image"
    echo "or do not set SKIP_DOWNLOAD=1."
    exit 1
fi

cp "$LEGATO_IMG" "$BUILD_DIR"
cp "$KERNEL_IMG" "$BUILD_DIR"
cp "$ROOTFS_IMG" "$BUILD_DIR"

cp $(dirname ${SCRIPT_PATH})/Dockerfile.${VIRT_TARGET_ARCH} ${BUILD_DIR}/Dockerfile

## Build image

cd $BUILD_DIR

docker build . | tee -a docker-build.log

BUILD_ID=$(tail -1 docker-build.log | awk '{print $3}')
if [ "${#BUILD_ID}" -gt 12 ]; then
    echo "Image build failed"
    exit 1
fi

LONG_BUILD_ID=$(docker inspect --type=image "$BUILD_ID" | jq -r .[0].Id)
if [ -z "$LONG_BUILD_ID" ]; then
    echo "Unable to get long ID for image $BUILD_ID"
    exit 1
fi

echo $LONG_BUILD_ID > image.id
