#!/bin/bash

set -e

function cleanup {
  rm -f /tmp/guest.in /tmp/guest.out
  rm -rf /tmp/qemu_mount
}


trap cleanup EXIT

mkfifo /tmp/guest.in /tmp/guest.out
mkdir -p /tmp/qemu_mount

cp ./Build/Stage1Apps/DEBUG_GCC5/X64/*.efi /tmp/qemu_mount

mkdir -p /tmp/qemu_mount/bmps
cp ./VulnerableCode/Stage1Apps/DisplayBMP/*.bmp /tmp/qemu_mount/bmps

qemu-system-x86_64  \
  -drive if=pflash,format=raw,readonly=on,file=./Build/OvmfX64/DEBUG_GCC5/FV/OVMF_CODE.fd \
  -drive if=pflash,format=raw,file=./Build/OvmfX64/DEBUG_GCC5/FV/OVMF_VARS.fd -hda fat:raw:rw:/tmp/qemu_mount  \
  -net none -serial mon:stdio -serial pipe:/tmp/guest -d pcall
