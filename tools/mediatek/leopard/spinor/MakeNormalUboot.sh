# !/bin/bash
#

u_boot_img_file=$1

[ -z "$u_boot_img_file" ] && u_boot_img_file=u-boot.lzma.img

echo "Make SPI NOR u-boot Block Size 65536 Bytes....\n"

dd if=/dev/zero of=u-boot-mtk.bin bs=1024 count=64 seek=0

dd if=$u_boot_img_file of=u-boot-mtk.bin bs=1024 seek=0 conv=notrunc

echo "Make Normal uboot u-boot-mtk.bin completed.\n"