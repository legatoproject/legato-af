#!/bin/bash

set -xe

if [ -z "$LEGATO_ROOT" ]; then
    echo "LEGATO_ROOT not set"
    exit 1
fi

. "$LEGATO_ROOT/framework/tools/scripts/shlib"

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

    # Use ONLY_PREPARE=1 to prevent the script from starting qemu.
    # Use OPT_PERSIST=0 to prevent the script from generating an empty userfs partition.
    ONLY_PREPARE=1 OPT_PERSIST=0 $BUILD_DIR/legato-qemu

    export KERNEL_IMG="$USER_DIR/img-virt-${VIRT_TARGET_ARCH}/kernel"
    export ROOTFS_IMG="$USER_DIR/img-virt-${VIRT_TARGET_ARCH}/rootfs.qcow2"

    export LEGATO_IMG="$USER_DIR/img-virt-${VIRT_TARGET_ARCH}/legato.squashfs"

    export QEMU_CONFIG="$USER_DIR/img-virt-${VIRT_TARGET_ARCH}/qemu-config"
fi

if [ ! -f "$LEGATO_IMG" ]; then
    echo "legato image ($LEGATO_IMG) not available, please build Legato before packaging the Docker image"
    echo "or do not set SKIP_DOWNLOAD=1."
    exit 1
fi

if [ -z "$USERFS_IMG" ] || [ ! -e "$USERFS_IMG" ]; then

    USERFS_SIZE=${USERFS_SIZE:-2G}
    USERFS_IMG=${USERFS_IMG:-"$BUILD_DIR/userfs.qcow2"}

    echo "userfs image ($USERFS_IMG) does not exist, creating one that contain sample apps."

    TARGET="virt_${VIRT_TARGET_ARCH}"
    FindToolchain

    FindTool "qemu-img" QEMU_IMG
    CheckRet

    FindTool "mkfs.ext4" MKFS_EXT4
    CheckRet

    set -e

    $QEMU_IMG create -f raw "${USERFS_IMG}.ext4" $USERFS_SIZE

    # Prepare content
    rm -rf "$BUILD_DIR/userfs"
    mkdir "$BUILD_DIR/userfs"

    # ... copy sample apps
    if ! cp -R "$LEGATO_ROOT/build/virt/samples" "$BUILD_DIR/userfs/"; then
        echo "Sample apps not available, did you run 'make tests_virt' to build sample and test code?"
        echo "Continuing without ..."
    fi

    # Generate an ext4 partition that will be used as /mnt/flash
    $MKFS_EXT4 -L userfs -d "$BUILD_DIR/userfs/" "${USERFS_IMG}.ext4"

    # Convert to qcow2 (mostly to save space)
    $QEMU_IMG convert -f raw -O qcow2 "${USERFS_IMG}.ext4" "$USERFS_IMG"

    rm -f "${USERFS_IMG}.ext4"

    set +e
fi

# Linux
cp "$KERNEL_IMG" "$BUILD_DIR"
cp "$ROOTFS_IMG" "$BUILD_DIR"

# Legato
cp "$LEGATO_IMG" "$BUILD_DIR"

# User FS
cp "$USERFS_IMG" "$BUILD_DIR"

# Config
cp "$QEMU_CONFIG" "$BUILD_DIR"

cp $(dirname ${SCRIPT_PATH})/Dockerfile.${VIRT_TARGET_ARCH} ${BUILD_DIR}/Dockerfile

## Build image

cd $BUILD_DIR

docker build . | tee -a docker-build.log
if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo "Image build failed"
    exit 1
fi

BUILD_ID=$(tail -1 docker-build.log | awk '{print $3}')
if [ -z "${BUILD_ID}" ] || [ "${#BUILD_ID}" -gt 12 ]; then
    echo "Invalid image ID"
    exit 1
fi

LONG_BUILD_ID=$(docker inspect --type=image "$BUILD_ID" | jq -r .[0].Id)
if [ -z "$LONG_BUILD_ID" ]; then
    echo "Unable to get long ID for image $BUILD_ID"
    exit 1
fi

echo "$LONG_BUILD_ID" > image.id

docker save -o virt.img "$LONG_BUILD_ID"

