#!/bin/bash

set -e

MOUNT_DIR="./tmp/mnt"
LOOP_DEVICE="/dev/loop20"
EMPTY_DISK_IMAGE="./vendor/grub/grub-disk.img"

# Clean up
mkdir -p "$MOUNT_DIR"
sudo umount "$MOUNT_DIR" &>/dev/null || true
sudo losetup -d "$LOOP_DEVICE" &>/dev/null || true

# Create blank disk image and partition
dd if=/dev/zero of="$EMPTY_DISK_IMAGE" bs=1M count=4
chmod 666 "$EMPTY_DISK_IMAGE"
echo -e "o\nn\np\n1\n2048\n\na\nt\n06\nw\n" | fdisk "$EMPTY_DISK_IMAGE"
sudo losetup "$LOOP_DEVICE" -P "$EMPTY_DISK_IMAGE"

# Create and mount fat16 filesystem
sudo mkfs.vfat -F16 -s1 "$LOOP_DEVICE"p1
sudo fsck -fy "$LOOP_DEVICE"p1
sudo mount -o sync "$LOOP_DEVICE"p1 "$MOUNT_DIR"
sleep 0.5

# Install grub
sudo grub-install --fonts= --themes= --locales=  --target=i386-pc --boot-directory="$MOUNT_DIR/boot/" "$LOOP_DEVICE"

# Unmount and cleanup
sudo umount "$MOUNT_DIR"
sudo losetup -d "$LOOP_DEVICE"
