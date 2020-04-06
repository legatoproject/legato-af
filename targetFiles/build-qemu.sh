#!/bin/bash

set -xe

SCRIPT_PATH="$( cd `dirname "$0"` && pwd )/`basename $0`"

# Determine LEGATO_ROOT if not set based on current directory
if [ -z "$LEGATO_ROOT" ]; then
    LEGATO_ROOT="$(dirname $SCRIPT_PATH)/.."
    if [ ! -e "${LEGATO_ROOT}/framework" ]; then
        echo "LEGATO_ROOT not set"
        exit 1
    fi

    LEGATO_ROOT="$(readlink -f "${LEGATO_ROOT}")"
fi

# Determine QEMU_TARGET if not set based on current directory
if [ -z "$QEMU_TARGET" ]; then
    if [ -n "$1" ]; then
        QEMU_TARGET="$1"
    elif [ -n "$LEGATO_TARGET" ]; then
        QEMU_TARGET="$LEGATO_TARGET"
    fi
fi

echo "Building for $QEMU_TARGET"

declare -a YOCTO_BUILD_DIRS=("build_bin" "build_src")
declare -a YOCTO_PLATFORMS
declare -a YOCTO_SYSROOTS

case "$QEMU_TARGET" in
    ar758x)
        YOCTO_PLATFORMS+=("swi-mdm9x28-ar758x-qemu")
        YOCTO_SYSROOTS+=("armv7a-neon-poky-linux-gnueabi")
        DOCKER_IMAGE_TYPE="build"
        ;;

    wp76xx)
        YOCTO_PLATFORMS+=("swi-mdm9x28-qemu")
        YOCTO_PLATFORMS+=("swi-mdm9x28-wp76xx-qemu")
        YOCTO_SYSROOTS+=("armv7a-neon-poky-linux-gnueabi")
        DOCKER_IMAGE_TYPE="build"
        ;;

    fx30-cat*)
        YOCTO_PLATFORMS+=("swi-mdm9x28-fx30-qemu")
        YOCTO_SYSROOTS+=("armv7a-neon-poky-linux-gnueabi")
        DOCKER_IMAGE_TYPE="build"
        if [[ "$QEMU_TARGET" == "fx30-cat1" ]]; then
            LEGATO_TARGET="wp76xx"
        elif [[ "$QEMU_TARGET" == "fx30-catm" ]]; then
            LEGATO_TARGET="wp77xx"
        else
            exit 1
        fi
        ;;

    virt-x86)
        VIRT_TARGET_ARCH="x86"
        YOCTO_BUILD_DIRS=("build_virt-${VIRT_TARGET_ARCH}")
        YOCTO_PLATFORMS+=("swi-virt-${VIRT_TARGET_ARCH}")
        YOCTO_SYSROOTS+=("i586-poky-linux" "core2-32-poky-linux")
        DOCKER_IMAGE_TYPE="alpine"
        BUILD_ARGS+=" --build-arg QEMU_ARCH=i386"
        ;;

    virt-arm)
        VIRT_TARGET_ARCH="arm"
        YOCTO_BUILD_DIRS=("build_virt-${VIRT_TARGET_ARCH}")
        YOCTO_PLATFORMS+=("swi-virt-${VIRT_TARGET_ARCH}")
        YOCTO_SYSROOTS+=("armv5e-poky-linux-gnueabi")
        DOCKER_IMAGE_TYPE="alpine"
        BUILD_ARGS+=" --build-arg QEMU_ARCH=arm"
        ;;

    *)
        echo "Target '$QEMU_TARGET' not supported"
        exit 1
        ;;
esac

export LEGATO_TARGET=${LEGATO_TARGET:-$QEMU_TARGET}
export TARGET=$LEGATO_TARGET
BUILD_ARGS+=" --build-arg LEGATO_TARGET=$LEGATO_TARGET"

. "$LEGATO_ROOT/framework/tools/scripts/shlib"

# Use Yocto & Legato from Yocto build if available
if [ -z "$YOCTO_DIR" ]; then
    for yocto_dir in "${YOCTO_BUILD_DIRS[@]}"; do
        if [ -e "$LEGATO_ROOT/../$yocto_dir" ]; then
            YOCTO_DIR="$LEGATO_ROOT/../$yocto_dir"
            break
        fi
    done
fi
if [ -e "$YOCTO_DIR" ]; then
    echo "Yocto build detected"

    for yocto_platform in "${YOCTO_PLATFORMS[@]}"; do
        # Deploy dir
        YOCTO_DEPLOY_DIR="${YOCTO_DEPLOY_DIR:-"${YOCTO_DIR}/tmp/deploy/images/${yocto_platform}"}"
        if [ -e "$YOCTO_DEPLOY_DIR" ]; then
            break
        fi
    done
    if [ ! -e "$YOCTO_DEPLOY_DIR" ]; then
        echo "Unable to find Yocto deploy directory"
        echo "Are you sure you built Yocto with QEMU=1 ?"
        exit 1
    fi

    for yocto_sysroot in "${YOCTO_SYSROOTS[@]}"; do
        LEGATO_BUILD_DIR="${YOCTO_DIR}/tmp/work/${yocto_sysroot}/legato-af/git-r0/legato-af/build/${LEGATO_TARGET}"
        if [ -e "$LEGATO_BUILD_DIR" ]; then
            break
        fi
    done
    if [ ! -e "$LEGATO_BUILD_DIR" ]; then
        # Search as a fallback
        RECIPE_ROOT="$(find "${YOCTO_DIR}/tmp/work/" -maxdepth 2 -name "legato-af" | head -1)"
        if [ -n "$RECIPE_ROOT" ]; then
            LEGATO_BUILD_DIR="${RECIPE_ROOT}/git-r0/legato-af/build/${LEGATO_TARGET}"
        fi
    fi
    if [ ! -e "$LEGATO_BUILD_DIR" ]; then
        echo "Unable to find Legato build directory in Yocto"
        unset LEGATO_BUILD_DIR
    fi

    LINUX_TARBALL_PATH="${YOCTO_DEPLOY_DIR}/img-${LEGATO_TARGET}.tar.bz2"
    if [ ! -e "$LINUX_TARBALL_PATH" ]; then
        echo "Yocto not built, $LINUX_TARBALL_PATH does not exist."
        exit 1
    fi

    export LEGATO_IMG="${YOCTO_DEPLOY_DIR}/legato-image.${LEGATO_TARGET}.squashfs"
    if [ ! -e "$LEGATO_IMG" ]; then
        echo "Unable to find Legato image as built from Yocto, assuming no Legato"
    fi
fi

if [ -z "$LEGATO_BUILD_DIR" ]; then
    LEGATO_BUILD_DIR="${LEGATO_ROOT}/build/${LEGATO_TARGET}"
fi

if [ ! -e "$LEGATO_BUILD_DIR" ]; then
    echo "Build directory does not exist at ${LEGATO_BUILD_DIR}"
    mkdir -p "${LEGATO_BUILD_DIR}"
fi

## Create build directory

if [ -z "$BUILD_DIR" ]; then
    BUILD_DIR="${LEGATO_BUILD_DIR}/docker"
fi
if [[ "$BUILD_DIR" != "/"* ]]; then
    BUILD_DIR="$PWD/$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"

## Prepare files to be packaged in Docker image

# Configuration Script
cp "$LEGATO_ROOT/framework/tools/scripts/legato-qemu" "$BUILD_DIR"

if [ -z "$USER_DIR" ]; then
    export USER_DIR="$BUILD_DIR"
fi

# Use local linux tarball if available
if [ -n "$LINUX_TARBALL_PATH" ]; then
    rm -rf "$BUILD_DIR/img-${QEMU_TARGET}"
    mkdir -p "$BUILD_DIR/img-${QEMU_TARGET}"

    tar jxvf "$LINUX_TARBALL_PATH" -C "$BUILD_DIR/img-${QEMU_TARGET}"

    SKIP_DOWNLOAD=1
fi

# Download Legato image & Linux image
if [[ "$SKIP_DOWNLOAD" != "1" ]]; then

    # Use ONLY_PREPARE=1 to prevent the script from starting qemu.
    # Use OPT_PERSIST=0 to prevent the script from generating an empty userfs partition.
    ONLY_PREPARE=1 OPT_PERSIST=0 $BUILD_DIR/legato-qemu
fi

if [ -e "${USER_DIR}/img-${QEMU_TARGET}/env" ]; then
    source "${USER_DIR}/img-${QEMU_TARGET}/env"
fi

export QEMU_CONFIG_JSON="${QEMU_CONFIG_JSON:-"$USER_DIR/img-${QEMU_TARGET}/qemu.json"}"

if [ -e "${QEMU_CONFIG_JSON}" ]; then

    get_var_from_json() {
        local env_var_name="$1"
        local json_key="$2"
        local is_path="true"
        local value

        if [ -n "${!env_var_name}" ]; then
            return
        fi

        if ! command -v jq >/dev/null; then
            echo "jq not available"
            exit 1
        fi

        value="$(jq -r "${json_key}" "${QEMU_CONFIG_JSON}" 2>/dev/null | grep -v null)"
        if [[ "$is_path" == "true" ]] && [ -n "$value" ]; then
            value="$(dirname "${QEMU_CONFIG_JSON}")/${value}"
        fi

        export ${env_var_name}="${value}"
    }

    set +e

    get_var_from_json KERNEL_IMG ".files.kernel.name"
    get_var_from_json DTB_FILE ".files.device_tree.name"
    get_var_from_json ROOTFS_IMG ".files.rootfs.name"
    get_var_from_json FLASH_FILE ".files.flash.name"

    get_var_from_json LEGATO_IMG ".files.legato.name"

    set -e
fi

export KERNEL_IMG="${KERNEL_IMG:-"$USER_DIR/img-${QEMU_TARGET}/kernel"}"
export ROOTFS_IMG="${ROOTFS_IMG:-"$USER_DIR/img-${QEMU_TARGET}/rootfs.qcow2"}"

export LEGATO_IMG="${LEGATO_IMG:-"$USER_DIR/img-${QEMU_TARGET}/legato.squashfs"}"

export QEMU_CONFIG="${QEMU_CONFIG:-"$USER_DIR/img-${QEMU_TARGET}/qemu-config"}"

if [ ! -f "$LEGATO_IMG" ] && [[ "$LEGATO_TARGET" == "virt"* ]]; then
    echo "legato image ($LEGATO_IMG) not available, please build Legato before packaging the Docker image"
    echo "or do not set SKIP_DOWNLOAD=1."
    exit 1
fi

if [ -z "$USERFS_IMG" ] || [ ! -e "$USERFS_IMG" ] && [[ "$LEGATO_TARGET" == "virt"* ]]; then

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
        echo "Target tools not available, did you run 'make ${LEGATO_TARGET}' to build the legato framework?"
        echo "Continuing without ..."
    fi

    # ... copy sample apps
    if ! cp -R "${LEGATO_BUILD_DIR}/samples" "$BUILD_DIR/userfs/"; then
        echo "Sample apps not available, did you run 'make samples_${LEGATO_TARGET}' to build sample apps?"
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
mkdir -p "$BUILD_DIR/staging/data/"

# Script
cp "$BUILD_DIR/legato-qemu" "$BUILD_DIR/staging/data/"

stage_file() {
    local env_var="$1"
    if [ -n "${!env_var}" ] && [ -e "${!env_var}" ]; then
        cp "${!env_var}" "$BUILD_DIR/staging/data/"
        BUILD_ARGS+=" --build-arg ${env_var}=/data/$(basename "${!env_var}")"
    fi
}

# Flash
stage_file FLASH_FILE

# Kernel
stage_file KERNEL_IMG
stage_file DTB_FILE

if [ ! -e "$FLASH_FILE" ]; then
    # Rootfs
    stage_file ROOTFS_IMG

    # Legato
    stage_file LEGATO_IMG

    # User FS
    stage_file USERFS_IMG
fi

# Config
stage_file QEMU_CONFIG
stage_file QEMU_CONFIG_JSON

# Shared Memory Dumps
SMEM_PATH="$LEGATO_ROOT/proprietary/sierra/smemDump/${LEGATO_TARGET}"
if [ -d "$SMEM_PATH" ]; then
    cp -v "$SMEM_PATH"/* "$BUILD_DIR/staging/data/"
fi

# QEmu sources
if [[ "$DOCKER_IMAGE_TYPE" == "build" ]]; then
    mkdir -p "${BUILD_DIR}/staging/qemu/"
    cp -R "${LEGATO_ROOT}/3rdParty/qemu/" "${BUILD_DIR}/staging/qemu/"
fi

cp "$(dirname ${SCRIPT_PATH})/Dockerfile.${DOCKER_IMAGE_TYPE}" "${BUILD_DIR}/staging/Dockerfile"

## Build image

cd $BUILD_DIR/staging/

docker build $BUILD_ARGS . 2>&1 | tee -a "$BUILD_DIR/docker-build.log"
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

docker save -o "$BUILD_DIR/qemu.img" "$LONG_BUILD_ID"
ln -sf "qemu.img" "$BUILD_DIR/virt.img"
