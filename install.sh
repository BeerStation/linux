#!/bin/bash

HOME_PATH=/media/spok
KERNEL=kernel8
#make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- bcm2711_defconfig

#edit .config
#CONFIG_DRM_TI_SN65DSI83=m

#make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image modules dtbs

#mount cm4 to pc (to /media/spok/boot and /media/spok/rootfs as example)

sudo env PATH=$PATH make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- INSTALL_MOD_PATH=$HOME_PATH/rootfs modules_install

echo "copying files...."
sudo cp $HOME_PATH/boot/$KERNEL.img $HOME_PATH/boot/$KERNEL-backup.img
sudo cp arch/arm64/boot/Image $HOME_PATH/boot/$KERNEL.img
sudo cp arch/arm64/boot/dts/broadcom/*.dtb $HOME_PATH/boot/
sudo cp arch/arm64/boot/dts/overlays/*.dtb* $HOME_PATH/boot/overlays/
sudo cp arch/arm64/boot/dts/overlays/README $HOME_PATH/boot/overlays/

echo "copy config file"
sudo cp $HOME_PATH/boot/config.txt $HOME_PATH/boot/config-backup.txt
sudo cp config.txt $HOME_PATH/boot/
echo "done!"
#sudo umount $HOME_PATH/boot
#sudo umount $HOME_PATH/rootfs
