#!/bin/bash

KER_NAME="Sooplus"
DAY=$(date +"%d")
MONTH=$(date +"%m")
BASE_KER_VER=$MONTH.$DAY
CODENAME="hammerhead"
AUTHOR="soorajj"
OUT_DIR=./kernel_flashable
IMG_OUT_DIR=./kernel_flashable/boot_image
TOOLCHAIN_DIR=~/android/toolchain/arm-cortex_a15-linux-gnueabihf-linaro_4.9.3-2015.01/bin/arm-cortex_a15-linux-gnueabihf-
INITRAMFS_TMP=./initramfs
INITRAMFS_SOURCE=~/android/ramdisk/
export LOCALVERSION="-"`echo $KER_NAME~$BASE_KER_VER`
export CROSS_COMPILE=$TOOLCHAIN_DIR
export ARCH=arm
export SUBARCH=arm
export KBUILD_BUILD_USER="soorajj"
export KBUILD_BUILD_HOST="me.com"
echo 
echo "Making defconfig"
DATE=$(date +"%d-%m-%Y-%H.%M.%S")
COMPILER="LINARO-4.9"
ARCHIVE_FILE="$KER_NAME~$BASE_KER_VER-$CODENAME-$COMPILER-$DATE"

make "cyanogenmod_hammerhead_defconfig"

echo "LOCALVERSION="$LOCALVERSION
echo "CROSS_COMPILE="$CROSS_COMPILE
echo "ARCH="$ARCH

echo "Start of build: $DATE...";
sleep 3
echo "kernel version: $KER_NAME~$BASE_KER_VER...";
script -q ~/android/logs/$ARCHIVE_FILE.txt -c "
make -j16 "

echo "Build completed."
echo "Copying zImage to root folder"
cp arch/arm/boot/zImage-dtb zImage
	# remove previous initramfs files
	if [ -d $INITRAMFS_TMP ]; then
		echo "***** Removing old temp initramfs_source *****";
		rm -rf $INITRAMFS_TMP;
	fi;

	mkdir -p $INITRAMFS_TMP;
	cp -ax $INITRAMFS_SOURCE/* $INITRAMFS_TMP;
	# clear git repository from tmp-initramfs
	if [ -d $INITRAMFS_TMP/.git ]; then
		rm -rf $INITRAMFS_TMP/.git;
	fi;
	
	# clear mercurial repository from tmp-initramfs
	if [ -d $INITRAMFS_TMP/.hg ]; then
		rm -rf $INITRAMFS_TMP/.hg;
	fi;

	# remove empty directory placeholders from tmp-initramfs
	find $INITRAMFS_TMP -name EMPTY_DIRECTORY -exec rm '{}' \;

	# remove more from from tmp-initramfs ...
	rm -f $INITRAMFS_TMP/update* >> /dev/null;
	
	echo "packing ramdisk"
	./utils/mkbootfs $INITRAMFS_TMP | gzip > ramdisk.gz
	
	echo "***** Ramdisk Generation Completed Successfully *****"

echo "making boot.img"
./utils/mkbootimg --kernel zImage --cmdline 'console=ttyHSL0,115200,n8 androidboot.hardware=hammerhead user_debug=31 msm_watchdog_v2.enable=1 androidboot.selinux=permissive' --base 0x00000000 --pagesize 2048 --ramdisk_offset 0x02900000 --tags_offset 0x02700000 --ramdisk ramdisk.gz --output boot.img
echo "copying boot.img to $IMG_OUT_DIR..."
mkdir -p $IMG_OUT_DIR
cp boot.img $IMG_OUT_DIR
echo "[BUILD]: Changing aroma version/data/device to: $ARCHIVE_FILE...";
sed -i "/ini_set(\"rom_name\",/c\ini_set(\"rom_name\", \""$KER_NAME"\");" $OUT_DIR/META-INF/com/google/android/aroma-config
sed -i "/ini_set(\"rom_version\",/c\ini_set(\"rom_version\", \""$BASE_KER_VER"\");" $OUT_DIR/META-INF/com/google/android/aroma-config
sed -i "/ini_set(\"rom_date\",/c\ini_set(\"rom_date\", \""$DATE"\");" $OUT_DIR/META-INF/com/google/android/aroma-config
sed -i "/ini_set(\"rom_device\",/c\ini_set(\"rom_device\", \""$CODENAME"\");" $OUT_DIR/META-INF/com/google/android/aroma-config
sed -i "/ini_set(\"rom_author\",/c\ini_set(\"rom_author\", \""$AUTHOR"\");" $OUT_DIR/META-INF/com/google/android/aroma-config

cd $OUT_DIR
find . -name '*zip' -exec cp '{}' ~/my-works/ \;
find . -name '*zip' -exec rm '{}' \;
zip -r `echo $ARCHIVE_FILE`.zip *
find . -name '*zip' -exec cp '{}' ~/my-works/ \;
echo "Flashable zip made."
echo "cleaning."
find . -name '*ko' -exec rm '{}' \;
find . -name 'zImage' -exec rm '{}' \;
cd ../
make "clean"
make "mrproper"
echo "Remove previous files which should regenerate every time"
rm -f arch/arm/boot/*.dtb >> /dev/null;
rm -f arch/arm/boot/*.cmd >> /dev/null;
rm -f arch/arm/boot/zImage >> /dev/null;
rm -f arch/arm/boot/zImage-dtb >> /dev/null;
rm -f arch/arm/boot/Image >> /dev/null;
rm -f zImage >> /dev/null;
rm -f zImage-dtb >> /dev/null;
rm -f boot.img >> /dev/null;
rm -f ramdisk.gz >> /dev/null;
git clean -f -d
git reset --hard
echo "everything completed."
echo "End of build: $DATE...";
exit
