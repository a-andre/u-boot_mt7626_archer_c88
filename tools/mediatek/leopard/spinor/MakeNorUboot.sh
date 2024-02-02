# !/bin/bash
#

u_boot_img_file=$1

[ -z "$u_boot_img_file" ] && u_boot_img_file=u-boot.img

echo "Make SPI NOR u-boot Block Size 65536 Bytes....\n"

dd if=/dev/zero of=u-boot-mtk.bin bs=256 count=496 seek=0

dd if=spl/u-boot-spl-mtk.bin of=u-boot-mtk.bin bs=256 seek=0 conv=notrunc

dd if=$u_boot_img_file of=u-boot-mtk.bin bs=256 seek=212 conv=notrunc

echo "Make SPI NOR u-boot-mtk.bin completed.\n"