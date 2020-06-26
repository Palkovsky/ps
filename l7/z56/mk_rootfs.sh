#!/bin/bash

IMG_SIZE=512M
BEFORE_CMD=""
CHROOT_CMD=""
AFTER_CMD=""

POSITIONAL=()
while [[ $# -gt 0 ]]
do
    key="$1"
    case $key in
        --before)
            BEFORE_CMD=$2
            shift ; shift
            ;;
        --chroot)
            CHROOT_CMD=$2
            shift ; shift
            ;;
        --after)
            AFTER_CMD=$2
            shift ; shift
            ;;
        *)
            POSITIONAL+=($1)
            shift
            ;;
    esac
done
set -- ${POSITIONAL[@]}


# Download and unpack Alpine Linux
wget -O rootfs.tar.gz http://dl-cdn.alpinelinux.org/alpine/v3.12/releases/x86_64/alpine-minirootfs-3.12.0-x86_64.tar.gz
mkdir rootfs/ && tar -C rootfs/ -xvf rootfs.tar.gz

# Create image file, mount it as a loopback device, format as ext4, copy rootfs.
rm rootfs.img ; fallocate -l $IMG_SIZE rootfs.img
losetup -fP rootfs.img
echo 'y' | mkfs.ext4 rootfs.img
umount /mnt ; mount -o loop rootfs.img /mnt
cp -r rootfs/* /mnt/

[ "$BEFORE_CMD" ] && /bin/bash -c "$BEFORE_CMD"

# Run stuff inside chroot
chroot /mnt /bin/sh <<EOF
export PATH=$PATH:/bin/

echo "::sysinit:/bin/mount -t proc none /proc" >> /etc/inittab
echo "::sysinit:/bin/mount -t sysfs none /sys" >> /etc/inittab

[ "$CHROOT_CMD" ] && /bin/ash -c "$CHROOT_CMD"
EOF


[ "$AFTER_CMD" ] && /bin/bash -c "$AFTER_CMD"

# Umount and cleanup.
umount /mnt
rm -rf rootfs rootfs.tar.gz
chmod 666 rootfs.img
