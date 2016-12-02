#!/bin/sh
PATH=${PATH}:${PWD}/tc/bin/
export ARCH=arm64
make CP8676_I02_defconfig ARCH=arm64 CROSS_COMPILE=aarch64-linux-android-
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-android-

