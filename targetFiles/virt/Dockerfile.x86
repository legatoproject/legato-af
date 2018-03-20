FROM alpine

ENV VIRT_TARGET_ARCH="x86" \
    QEMU_ARCH="i386"

RUN ( \
        apk add --no-cache qemu-system-${QEMU_ARCH} \
    )

ADD legato-qemu \
    kernel \
    rootfs.qcow2 \
    legato.squashfs \
    userfs.qcow2 \
    qemu-config \
        /legato/

ENV KERNEL_IMG=/legato/kernel \
    ROOTFS_IMG=/legato/rootfs.qcow2 \
    LEGATO_IMG=/legato/legato.squashfs \
    USERFS_IMG=/legato/userfs.qcow2 \
    QEMU_CONFIG=/legato/qemu-config \
    PID_FILE=/tmp/qemu.pid

# No offset at the port level, so:
# - telnet is available on 21
# - SSH on 22
# - simu control on 5000
ENV OFFSET_PORT 0

VOLUME /legato

EXPOSE 21 \
       22 \
       5000 \
       8080 \
       8443

ENTRYPOINT ["/legato/legato-qemu"]

