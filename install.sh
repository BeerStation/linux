#!/bin/bash

BASE_PATH=/media/eyalfish23
KERNEL=kernel8
#make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- bcm2711_defconfig

#edit .config
#CONFIG_DRM_TI_SN65DSI83=m

#make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image modules dtbs

#mount cm4 to pc (to /media/spok/boot and /media/spok/rootfs as example)

sudo env PATH=$PATH make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- INSTALL_MOD_PATH=$BASE_PATH/rootfs modules_install

echo "copying files...."
sudo cp $BASE_PATH/boot/$KERNEL.img $BASE_PATH/boot/$KERNEL-backup.img
sudo cp arch/arm64/boot/Image $BASE_PATH/boot/$KERNEL.img
sudo cp arch/arm64/boot/dts/broadcom/*.dtb $BASE_PATH/boot/
sudo cp arch/arm64/boot/dts/overlays/*.dtb* $BASE_PATH/boot/overlays/
sudo cp arch/arm64/boot/dts/overlays/README $BASE_PATH/boot/overlays/

echo "copy config file"
sudo cp $BASE_PATH/boot/config.txt $BASE_PATH/boot/config-backup.txt
sudo cp config.txt $BASE_PATH/boot/
echo "done!"
#sudo umount /media/spok/boot
#sudo umount /media/spok/rootfs
