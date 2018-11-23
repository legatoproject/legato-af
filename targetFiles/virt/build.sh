#!/bin/bash

set -xe

SCRIPT_PATH="$( cd `dirname "$0"` && pwd )/`basename $0`"
if [ -L "$SCRIPT_PATH" ]; then
    SCRIPT_PATH=$(readlink $SCRIPT_PATH)
fi

if [ -z "$LEGATO_ROOT" ]; then
    LEGATO_ROOT="$(dirname $SCRIPT_PATH)/../.."
    if [ ! -e "${LEGATO_ROOT}/framework" ]; then
        echo "LEGATO_ROOT not set"
        exit 1
    fi

    LEGATO_ROOT="$(readlink -f "${LEGATO_ROOT}")"
fi

. "$LEGATO_ROOT/framework/tools/scripts/shlib"

VIRT_TARGET_ARCH=${VIRT_TARGET_ARCH:-x86}
TARGET="virt-${VIRT_TARGET_ARCH}"

# Use Yocto & Legato from Yocto build if available
YOCTO_DIR=${YOCTO_DIR:-"$LEGATO_ROOT/../build_virt-${VIRT_TARGET_ARCH}"}
if [ -e "$YOCTO_DIR" ]; then
    echo "Yocto build detected"

    # Deploy dir
    YOCTO_DEPLOY_DIR="${YOCTO_DEPLOY_DIR:-"${YOCTO_DIR}/tmp/deploy/images/swi-virt-${VIRT_TARGET_ARCH}"}"

    # Legato build dir
    if [[ "$VIRT_TARGET_ARCH" == "x86" ]] \
       && [ -e "${YOCTO_DIR}/tmp/work/i586-poky-linux/legato-af/git-r0/legato-af/build/${PLATFORM}" ]; then
        LEGATO_BUILD_DIR="${YOCTO_DIR}/tmp/work/i586-poky-linux/legato-af/git-r0/legato-af/build/${PLATFORM}"
    elif [[ "$VIRT_TARGET_ARCH" == "arm" ]] \
       && [ -e "${YOCTO_DIR}/tmp/work/armv5e-poky-linux-gnueabi/legato-af/git-r0/legato-af" ]; then
        LEGATO_BUILD_DIR="${YOCTO_DIR}/tmp/work/armv5e-poky-linux-gnueabi/legato-af/git-r0/legato-af/build/${PLATFORM}"
    else
        # Search as a fallback
        RECIPE_ROOT="$(find "${YOCTO_DIR}/tmp/work/" -maxdepth 2 -name "legato-af" | head -1)"
        if [ -n "$RECIPE_ROOT" ]; then
            LEGATO_BUILD_DIR="${RECIPE_ROOT}/git-r0/legato-af/build/${PLATFORM}"
            if [ ! -e "$LEGATO_BUILD_DIR" ]; then
                unset LEGATO_BUILD_DIR
            fi
        fi
    fi

    LINUX_TARBALL_PATH="${YOCTO_DEPLOY_DIR}/img-virt-${VIRT_TARGET_ARCH}.tar.bz2"
    if [ ! -e "$LINUX_TARBALL_PATH" ]; then
        echo "Yocto not built, $LINUX_TARBALL_PATH does not exist."
        exit 1
    fi

    export LEGATO_IMG="${YOCTO_DEPLOY_DIR}/legato-image.virt-${VIRT_TARGET_ARCH}.squashfs"
    if [ ! -e "$LEGATO_IMG" ]; then
        echo "Unable to find Legato image as built from Yocto"
        exit 1
    fi
fi

if [ -n "$LEGATO_BUILD_DIR" ]; then
    if [ ! -e "$LEGATO_BUILD_DIR" ]; then
        echo "Build directory does not exist at ${LEGATO_BUILD_DIR}"
        exit 1
    fi
else
    LEGATO_BUILD_DIR="$LEGATO_ROOT/build/${TARGET}"
fi

## Create build directory

if [ -z "$BUILD_DIR" ]; then
    BUILD_DIR="${LEGATO_BUILD_DIR}/docker"
fi

mkdir -p $BUILD_DIR

## Prepare files to be packaged in Docker image

# Configuration Script
cp "$LEGATO_ROOT/framework/tools/scripts/legato-qemu" "$BUILD_DIR"

if [ -z "$USER_DIR" ]; then
    export USER_DIR="$BUILD_DIR"
fi

# Use local linux tarball if available
if [ -n "$LINUX_TARBALL_PATH" ]; then
    rm -rf "$BUILD_DIR/img-virt-${VIRT_TARGET_ARCH}"
    mkdir -p "$BUILD_DIR/img-virt-${VIRT_TARGET_ARCH}"

    tar jxvf $LINUX_TARBALL_PATH -C "$BUILD_DIR/img-virt-${VIRT_TARGET_ARCH}"

    SKIP_DOWNLOAD=1
fi

# Download Legato image & Linux image
if [ -z "$SKIP_DOWNLOAD" ]; then

    # Use ONLY_PREPARE=1 to prevent the script from starting qemu.
    # Use OPT_PERSIST=0 to prevent the script from generating an empty userfs partition.
    ONLY_PREPARE=1 OPT_PERSIST=0 $BUILD_DIR/legato-qemu
fi

export KERNEL_IMG="${KERNEL_IMG:-"$USER_DIR/img-virt-${VIRT_TARGET_ARCH}/kernel"}"
export ROOTFS_IMG="${ROOTFS_IMG:-"$USER_DIR/img-virt-${VIRT_TARGET_ARCH}/rootfs.qcow2"}"

export LEGATO_IMG="${LEGATO_IMG:-"$USER_DIR/img-virt-${VIRT_TARGET_ARCH}/legato.squashfs"}"

export QEMU_CONFIG="${QEMU_CONFIG:-"$USER_DIR/img-virt-${VIRT_TARGET_ARCH}/qemu-config"}"

if [ ! -f "$LEGATO_IMG" ]; then
    echo "legato image ($LEGATO_IMG) not available, please build Legato before packaging the Docker image"
    echo "or do not set SKIP_DOWNLOAD=1."
    exit 1
fi

if [ -z "$USERFS_IMG" ] || [ ! -e "$USERFS_IMG" ]; then

    USERFS_SIZE=${USERFS_SIZE:-2G}
    USERFS_IMG=${USERFS_IMG:-"$BUILD_DIR/userfs.qcow2"}

    echo "userfs image ($USERFS_IMG) does not exist, creating one that contain sample apps."

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

    # ... copy tools apps
    if ! cp -R "${LEGATO_BUILD_DIR}/tools" "$BUILD_DIR/userfs/"; then
        echo "Target tools not available, did you run 'make virt-${VIRT_TARGET_ARCH}' to build the legato framework?"
        echo "Continuing without ..."
    fi

    # ... copy sample apps
    if ! cp -R "${LEGATO_BUILD_DIR}/samples" "$BUILD_DIR/userfs/"; then
        echo "Sample apps not available, did you run 'make samples_virt-${VIRT_TARGET_ARCH}' to build sample apps?"
        echo "Continuing without ..."
    fi

    # Generate an ext4 partition that will be used as /mnt/flash
    $MKFS_EXT4 -L userfs -d "$BUILD_DIR/userfs/" "${USERFS_IMG}.ext4"

    # Convert to qcow2 (mostly to save space)
    $QEMU_IMG convert -f raw -O qcow2 "${USERFS_IMG}.ext4" "$USERFS_IMG"

    rm -f "${USERFS_IMG}.ext4"

    set +e
fi

# Prepare staging area

rm -rf "$BUILD_DIR/staging"
mkdir "$BUILD_DIR/staging"

# Script
cp "$BUILD_DIR/legato-qemu" "$BUILD_DIR/staging/legato-qemu"

# Linux
cp "$KERNEL_IMG" "$BUILD_DIR/staging/kernel"
cp "$ROOTFS_IMG" "$BUILD_DIR/staging/rootfs.qcow2"

# Legato
cp "$LEGATO_IMG" "$BUILD_DIR/staging/legato.squashfs"

# User FS
cp "$USERFS_IMG" "$BUILD_DIR/staging/userfs.qcow2"

# Config
cp "$QEMU_CONFIG" "$BUILD_DIR/staging/qemu-config"

cp "$(dirname ${SCRIPT_PATH})/Dockerfile.${VIRT_TARGET_ARCH}" "${BUILD_DIR}/staging/Dockerfile"

## Build image

cd $BUILD_DIR/staging/

docker build . | tee -a "$BUILD_DIR/docker-build.log"
if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo "Image build failed"
    exit 1
fi

BUILD_ID=$(tail -1 "$BUILD_DIR/docker-build.log" | awk '{print $3}')
if [ -z "${BUILD_ID}" ] || [ "${#BUILD_ID}" -gt 12 ]; then
    echo "Invalid image ID"
    exit 1
fi

LONG_BUILD_ID=$(docker inspect --type=image --format='{{.Id}}' "$BUILD_ID")
if [ -z "$LONG_BUILD_ID" ]; then
    echo "Unable to get long ID for image $BUILD_ID"
    exit 1
fi

echo "$LONG_BUILD_ID" > "$BUILD_DIR/image.id"

docker save -o "$BUILD_DIR/virt.img" "$LONG_BUILD_ID"

