#!/bin/sh
CURR_DIR=$(cd $(dirname $0) && pwd)
TOOLCHAIN_NAME=buildroot-gcc492_arm
TOOLCHAIN_PATH=${CURR_DIR}/../toolchain
tar xjf ${TOOLCHAIN_PATH}/${TOOLCHAIN_NAME}.tar.bz2 -C ${TOOLCHAIN_PATH}
./make_config.sh
make silentconfig
# absolute path
make CROSS_COMPILE_PATH=${TOOLCHAIN_PATH}/${TOOLCHAIN_NAME}/usr/bin
cp -pr u-boot-mtk.bin uboot.bin
