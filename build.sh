#!/bin/bash

KERNEL=kernel8
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- bcm2711_defconfig

#edit .config
#CONFIG_DRM_TI_SN65DSI83=m
#CONFIG_LOCALVERSION="-v8"
#CONFIG_LOCALVERSION_AUTO
#CONFIG_GPIO_TAPS_IO_SAMPLER=y"

sed -i '/CONFIG_LOCALVERSION/c\CONFIG_LOCALVERSION=\"-v8-station\"' .config
sed -i '/CONFIG_DRM_TI_SN65DSI83/c\CONFIG_DRM_TI_SN65DSI83=m' .config
sed -i '/^CONFIG_GPIO_RASPBERRYPI_EXP/a\CONFIG_GPIO_TAPS_IO_SAMPLER=y' .config
sed -i '/^CONFIG_LOCALVERSION/a\CONFIG_LOCALVERSION_AUTO=y' .config

make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image modules dtbs

#mount cm4 to pc (to /media/spok/boot and /media/spok/rootfs as example)

#sudo env PATH=$PATH make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- INSTALL_MOD_PATH=/media/spok/rootfs modules_install

#sudo cp /media/spok/boot/$KERNEL.img /media/spok/boot/$KERNEL-backup.img
#sudo cp arch/arm64/boot/Image /media/spok/boot/$KERNEL.img
#sudo cp arch/arm64/boot/dts/broadcom/*.dtb /media/spok/boot/
#sudo cp arch/arm64/boot/dts/overlays/*.dtb* /media/spok/boot/overlays/
#sudo cp arch/arm64/boot/dts/overlays/README /media/spok/boot/overlays/
#sudo umount /media/spok/boot
#sudo umount /media/spok/rootfs
